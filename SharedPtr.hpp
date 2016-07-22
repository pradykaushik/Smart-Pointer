#ifndef SHARED_PTR_HEADER
#define SHARED_PTR_HEADER

#include <iostream>
using namespace std;

namespace cs540{

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	template <typename T>
	class SharedPtr;

	template <typename U>
	void destructor(void* dobj){
		delete static_cast<U*>(dobj);
	}


	class SharedPtrData{
		public:
			SharedPtrData() : ref(nullptr), ref_count(0){}
			SharedPtrData(void* obj) : ref(obj), ref_count(1){}
			SharedPtrData(const void* obj) : ref(const_cast<void*>(obj)), ref_count(1){}

			~SharedPtrData() {
				dptr = NULL;
				ref = NULL; // ref would already have been deleted.
			}

		public:
			void* ref;
			int ref_count;
			void (*dptr)(void*);
	};

	template <typename T>
	class SharedPtr{

		public:
			SharedPtr() : data(new SharedPtrData()){}

			template <typename U>
			explicit SharedPtr(U* uobj){
				pthread_mutex_lock(&mutex);
				data = new SharedPtrData(uobj);
				data->dptr = destructor<U>;
				pthread_mutex_unlock(&mutex);
			}

			template <typename U>
			SharedPtr(SharedPtrData* spdtr, U* obj) : data(spdtr){}

			SharedPtr(const SharedPtr& sptr){
				pthread_mutex_lock(&mutex);
				data = sptr.data;
				if(data->ref != nullptr) ++data->ref_count;
				pthread_mutex_unlock(&mutex);
			}

			template<typename U>
			SharedPtr(const SharedPtr<U>& sptr){
				pthread_mutex_lock(&mutex);
				data = const_cast<SharedPtrData*>(sptr.data);
				if(data->ref != nullptr) ++data->ref_count;
				pthread_mutex_unlock(&mutex);
			}

			SharedPtr(SharedPtr&& sptr){
				pthread_mutex_lock(&mutex);
				data = new SharedPtrData(std::move(*sptr.data));
				sptr.data = nullptr;
				pthread_mutex_unlock(&mutex);
			}
			template <typename U>
			SharedPtr(SharedPtr<U>&& sptr){
				pthread_mutex_lock(&mutex);
				data = new SharedPtrData(std::move(*sptr.data));
				sptr.data = nullptr;
				pthread_mutex_unlock(&mutex);
			}

			SharedPtr& operator=(const SharedPtr& sptr){
				pthread_mutex_lock(&mutex);
				if(data->ref != nullptr) --data->ref_count;
				data = sptr.data;
				if(data->ref != nullptr) ++data->ref_count;
				pthread_mutex_unlock(&mutex);
				return *this;
			}

			template <typename U>
			SharedPtr<T>& operator=(const SharedPtr<U>& sptr){
				pthread_mutex_lock(&mutex);
				if(data->ref != nullptr) --data->ref_count;
                                data = sptr.data;
                                if(data->ref != nullptr) ++data->ref_count;
				pthread_mutex_unlock(&mutex);
                                return *this;
			}

			SharedPtr& operator=(SharedPtr&& sptr){
				pthread_mutex_lock(&mutex);
				data = std::move(sptr.data);
				sptr.data->ref = nullptr;
				sptr.data->ref_count = 0;
				pthread_mutex_unlock(&mutex);
				return *this;
			}

			template <typename U>
			SharedPtr<T>& operator=(SharedPtr<U>&& sptr){
				pthread_mutex_lock(&mutex);
				data = std::move(sptr.data);
				sptr.data->ref = nullptr;
				sptr.data->ref_count = 0;
				pthread_mutex_unlock(&mutex);
				return *this;
			}

			~SharedPtr(){
				pthread_mutex_lock(&mutex);
				if(data->ref != nullptr){
					if (data->ref_count != 0){
						--data->ref_count;
						if(data->ref_count <= 0) data->dptr(data->ref);
						data->ref = nullptr;
					}
				}
				pthread_mutex_unlock(&mutex);
			}

			void reset(){
				pthread_mutex_lock(&mutex);
				if(data->ref != nullptr)
					--data->ref_count;
				data->ref = nullptr;
				pthread_mutex_unlock(&mutex);
			}

			template <typename U>
			void reset(U* obj){
				pthread_mutex_lock(&mutex);
				if(data->ref != nullptr){
					--data->ref_count;
					if(data->ref_count <= 0) data->dptr(data->ref);
					data->ref = nullptr;
				}
				data->ref = obj;
				data->ref_count = 1;
				data->dptr = destructor<U>;
				pthread_mutex_unlock(&mutex);
			}

			T* get() { return static_cast<T*>(data->ref); }
			T& operator*() const { return *static_cast<T*>(data->ref);}
			T* operator->() const { return static_cast<T*>(data->ref);}
			explicit operator bool() const{ return data->ref != nullptr;}

			template <typename T1, typename T2>
			friend bool operator==(const SharedPtr<T1>&, const SharedPtr<T2>&);
			template <typename T1>
			friend bool operator==(const SharedPtr<T> &, std::nullptr_t);
			template <typename T1>
			friend bool operator==(std::nullptr_t, const SharedPtr<T> &);
			template <typename T1, typename T2>
			friend bool operator!=(const SharedPtr<T1>&, const SharedPtr<T2> &);
			template <typename T1>
			friend bool operator!=(const SharedPtr<T1> &, std::nullptr_t);
			template <typename T1>
			friend bool operator!=(std::nullptr_t, const SharedPtr<T1> &);
			template <typename T1, typename U>
			SharedPtr<T1> static_pointer_cast(const SharedPtr<U>&);
			template <typename T1, typename U>
			SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<U>&);

		public:
			SharedPtrData* data;
	};

	template <typename T1, typename T2>
	bool operator==(const SharedPtr<T1>& sptr1, const SharedPtr<T2>& sptr2){
		return sptr1.data->ref == sptr2.data->ref;
	}
	
	template <typename T1>
	bool operator==(const SharedPtr<T1>& sptr, std::nullptr_t nptr){
		return sptr.data->ref == nptr;
	}

	template <typename T1>
	bool operator==(std::nullptr_t nptr, const SharedPtr<T1>& sptr){
		return sptr == nptr;
	}

	template <typename T1, typename T2>
	bool operator!=(const SharedPtr<T1>& sptr1, const SharedPtr<T2>& sptr2){
		return !(sptr1 == sptr2);
	}

	template <typename T1>
	bool operator!=(const SharedPtr<T1>& sptr, std::nullptr_t nptr){
		return !(sptr == nptr);
	}

	template <typename T1>
	bool operator!=(std::nullptr_t nptr, const SharedPtr<T1>& sptr){
		return sptr != nptr;
	}

	template <typename T1, typename U>
	SharedPtr<T1> static_pointer_cast(const SharedPtr<U>& sptr){
		T1* casted_ptr = static_cast<T1*>(sptr.data->ref);
		return SharedPtr<T1>(sptr.data, casted_ptr);
	}

	template <typename T1, typename U>
	SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<U>& sptr){
		auto casted_ptr = dynamic_cast<T1*>(static_cast<U*>(sptr.data->ref));
		if(casted_ptr) return SharedPtr<T1>(sptr.data, casted_ptr);
		return SharedPtr<T1>();
	}

}

#endif

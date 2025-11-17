#ifndef GPS_RESULT_H
#define GPS_RESULT_H

#include <cstring>

namespace gps {

	static int cntr = 0;

	template<typename T>
	class result {
		union RESULT_UNION {
			T value;
			const char* err_msg;
			RESULT_UNION() : err_msg("unitialized result") {

			}
			RESULT_UNION(const T& ivalue) : value(ivalue) {

			}
			RESULT_UNION(T&& ivalue) : value(std::forward<T>(ivalue)) {

			}
			RESULT_UNION(const char* imsg) : err_msg(imsg) {

			}
			~RESULT_UNION() {
				//if (err_msg)
				//	delete[] err_msg;
			}
		} _res;
		bool status = false;

		int _id = cntr++;

		static char* make_copy_str(const char* str) {
			int len = strlen(str) + 1;
			char* temp = new char[len];
			memcpy(temp, str, len - 1);
			temp[len - 1] = '\0';
			return temp;
		}
		result(const result<T>& r) = delete;
		result<T>& operator=(const result<T>& r) = delete;

	public:
		result(const T& val) : _res(val), status(true) {

		}
		result(result<T>&& r) : status(r.status) {
			if (r.status)
				_res.value = std::move(r.GetValue());
			else {
				_res.err_msg = make_copy_str(r.GetError());
			}
		}

		result<T>& operator=(result<T>&& r) noexcept {
			bool was_status = status;

			status = r.status;
			if (r.status)
				_res.value = std::move(r.GetValue());
			else {
				if(r.GetError() != nullptr)
					_res.err_msg = make_copy_str(r.GetError());
				else 
					_res.err_msg = "invalid move";
			}
			return *this;
		}

		result(T&& val) : _res(std::forward<T>(val)), status(true) {

		}
		result(const char* msg) : _res(make_copy_str(msg)), status(false) {

		}
		bool isOk() const {
			return status;
		}
		T& GetValue() {
			return _res.value;
		}
		const char* GetError() {
			return _res.err_msg;
		}
		~result() {
			if (!status)
				if (_res.err_msg)
					delete[] _res.err_msg;
		}
	};

}
#endif // !GPS_RESULT_H

#ifndef __BSW_ERROR_HPP__
#define __BSW_ERROR_HPP__

#include "predef.h"

#include <string>
#include <stdexcept>

#include "macros.h"


namespace bsw
{
    std::string error_to_string (long error_code);
    void raise_system_error (const char* fn_name, const char* src, int line_num);
    void raise_system_error (const char* fn_name, const char* src, int line_num, long error_code);


	namespace detail
	{
		struct default_predicate
		{
			template <class T>
			static bool wrong(const T& obj)
			{
				return !obj;
			}
		};

		struct always_wrong_predicate
		{
			template <class T>
			static bool wrong(const T& obj)
			{
				return true;
			}
		};

		template <typename Ex>
		struct generic_raiser
		{
			template <class T>
			static void raise(const T&, const std::string& message, const char* locus)
			{
				throw Ex(message + '\n' + locus);
			}
		};

		struct default_raiser
		{
			template <class T>
			static void raise(const T&, const std::string& message, const char* locus)
			{
				throw std::runtime_error(message + '\n' + locus);
			}
		};

		template<typename Ref, typename P, typename R>
		class enforcer
		{
		public:
			enforcer(Ref t, const char* locus)
				: t_(t), locus_(P::wrong(t) ? locus : nullptr)
			{
			}

			Ref operator*() const
			{
				if (locus_)
				{
					R::raise(t_, msg_, locus_);
				}
				return t_;
			}

			template <class MsgType>
			enforcer& operator()(const MsgType& msg)
			{
				if (locus_)
				{
					// Here we have time; an exception will be thrown
					std::ostringstream ss;
					ss << msg;
					msg_ += ss.str();
				}
				return *this;
			}

		private:
			Ref t_;
			std::string msg_;
			const char* const locus_;
		};

		template <class P, class R, typename T>
		inline enforcer<const T&, P, R> make_enforcer(const T& t, const char* locus)
		{
			return enforcer<const T&, P, R>(t, locus);
		}

		template <class P, class R, typename T>
		inline enforcer<T&, P, R> make_enforcer(T& t, const char* locus)
		{
			return enforcer<T&, P, R>(t, locus);
		}
	}
} // ns bsw

#define RAISE_SYSTEM_ERROR(FN_NAME) bsw::raise_system_error (FN_NAME, __FILE__, __LINE__)
#define RAISE_SYSTEM_ERROR_WITH_CODE(FN_NAME, CODE) bsw::raise_system_error (FN_NAME, __FILE__, __LINE__, CODE)


#define ENFORCE(exp) \
    *bsw::detail::make_enforcer<bsw::detail::default_predicate, bsw::detail::default_raiser>((exp), \
	"Expression '" #exp "' failed in '" __FILE__ "', line: " STRINGIZE(__LINE__))

#define THROW\
	*bsw::detail::make_enforcer<bsw::detail::always_wrong_predicate,\
	 bsw::detail::default_raiser>(false, \
	"std::runtime_error raised in '" __FILE__ "', line: " STRINGIZE(__LINE__))

#define THROW_EX(TYPE)\
	*bsw::detail::make_enforcer<bsw::detail::always_wrong_predicate,\
	 bsw::detail::generic_raiser<TYPE>>(false, \
	STRINGIZE(TYPE) " raised in '" __FILE__ "', line: " STRINGIZE(__LINE__))

#endif


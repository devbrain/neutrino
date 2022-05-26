#ifndef __PEFILE_CLR_TABLES_HELPER_HPP__
#define __PEFILE_CLR_TABLES_HELPER_HPP__

#include "type_list.hpp"
#include "istream_wrapper.hpp"

namespace pefile
{
	namespace detail
	{
		struct CLR_HEAP_STRINGS
		{
			static constexpr uint8_t StreamOffsetSizeMask = 0x01;
		};

		struct CLR_HEAP_GUIDS
		{
			static constexpr uint8_t StreamOffsetSizeMask = 0x02;
		};

		struct CLR_HEAP_BLOBS
		{
			static constexpr uint8_t StreamOffsetSizeMask = 0x04;
		};

		template <typename T>
		struct CLR_HEAP_INDEX
		{
			static size_t size(uint8_t StreamOffsetSize)
			{
				if ((StreamOffsetSize & T::StreamOffsetSizeMask) == T::StreamOffsetSizeMask)
				{
					return sizeof(uint32_t);
				}
				return sizeof(uint16_t);
			}

			static uint32_t read(uint8_t StreamOffsetSize, const char* buff)
			{
				union
				{
					const char*     bytes;
					const uint16_t* words;
					const uint32_t* dwords;
				} u;
				u.bytes = buff;
				if ((StreamOffsetSize & T::StreamOffsetSizeMask) == T::StreamOffsetSizeMask)
				{
					return *u.dwords;
				}
				return ((*u.words) & 0x0000FFFF);
			}
		};
		// =================================================================
		template <typename T>
		struct tuple_type_cnv
		{
			using type = std::tuple<T>;
		};
		template <typename T>
		struct tuple_type_cnv <CLR_HEAP_INDEX<T>>
		{
			using type = std::tuple<uint32_t>;
		};


		template <typename H>
		struct tuple_type_cnv <typelist::tlist<H>>
		{
			using type = typename tuple_type_cnv<typename H::type>::type;
		};
		template<typename, typename>
		struct append_to_type_seq { };

		template<typename T, typename... Ts>
		struct append_to_type_seq<std::tuple<T>, std::tuple<Ts...>>
		{
			using type = std::tuple<T, Ts...>;
		};

		template<typename T>
		struct append_to_type_seq<std::tuple<T>, std::tuple<>>
		{
			using type = std::tuple<T>;
		};

		template <typename Head, typename ...Tail>
		struct tuple_type_cnv <typelist::tlist<Head, Tail...>>
		{
			using A = typename tuple_type_cnv<typename Head::type>::type;
			using type = typename append_to_type_seq <A,
				typename tuple_type_cnv<typelist::tlist<Tail...>>::type>::type;
		};
		// ======================================================================
		template <typename T>
		struct reader_impl
		{
			static size_t size(uint8_t /*StreamOffsetSize*/)
			{
				return sizeof(T);
			}

			static T read(uint8_t /*StreamOffsetSize*/, const char* buff)
			{
				union
				{
					const char* bytes;
					const T* words;
				} u;
				u.bytes = buff;
				return *u.words;
			}
		};
		template <typename T>
		struct reader_impl <CLR_HEAP_INDEX<T>>
		{
			static size_t size(uint8_t StreamOffsetSize)
			{
				return CLR_HEAP_INDEX<T>::size(StreamOffsetSize);
			}

			static uint32_t read(uint8_t StreamOffsetSize, const char* buff)
			{
				return CLR_HEAP_INDEX<T>::read(StreamOffsetSize, buff);
			}
		};

		template <typename ROW, typename INSTANCE, std::size_t INDEX>
		struct elem_reader
		{
			using input_type = typename typelist::tlist_type_at <INDEX, ROW>::type;
			static void read(INSTANCE& instance, uint8_t StreamOffsetSize, bsw::istream_wrapper_c& is)
			{
				auto x = reader_impl<typename input_type::type>::read(StreamOffsetSize, is.rd_ptr());
				is.advance(reader_impl<typename input_type::type>::size(StreamOffsetSize));
				std::get <INDEX>(instance) = x;
			}
		};

		template <typename ROW, typename INSTANCE, std::size_t INDEX, bool ISLAST>
		struct inst_reader_impl
		{
			static void read(INSTANCE& instance, uint8_t StreamOffsetSize, bsw::istream_wrapper_c& is)
			{
				elem_reader<ROW, INSTANCE, INDEX>::read(instance, StreamOffsetSize, is);
				inst_reader_impl < ROW, INSTANCE, INDEX + 1, INDEX + 1 < ROW::size() > ::read(instance, StreamOffsetSize, is);
			}
		};
		template <typename ROW, typename INSTANCE, std::size_t INDEX>
		struct inst_reader_impl <ROW, INSTANCE, INDEX, false>
		{
			static void read(INSTANCE& instance, uint8_t StreamOffsetSize, bsw::istream_wrapper_c& is)
			{
			}
		};
		// =================================================================================
		template <typename T>
		struct memory_size_impl
		{
			static size_t eval(uint8_t /*StreamOffsetSize*/)
			{
				return sizeof(T);
			}
		};

		template <typename T>
		struct memory_size_impl<CLR_HEAP_INDEX<T>>
		{
			static size_t eval(uint8_t StreamOffsetSize)
			{
				return CLR_HEAP_INDEX<T>::size(StreamOffsetSize);
			}
		};

		template <typename T>
		struct memory_size_impl<typelist::tlist<T>>
		{
			static size_t eval(uint8_t StreamOffsetSize)
			{
				return memory_size_impl<typename T::type>::eval(StreamOffsetSize);
			}
		};

		template <typename H, typename... Ts>
		struct memory_size_impl <typelist::tlist<H, Ts...>>
		{
			static size_t eval(uint8_t StreamOffsetSize)
			{
				return memory_size_impl<typename H::type>::eval(StreamOffsetSize) +
					memory_size_impl<typelist::tlist<Ts...>>::eval(StreamOffsetSize);
			}
		};

	} // ns detail
	template <typename T>
	std::size_t memory_size(uint8_t StreamOffsetSize)
	{
		return detail::memory_size_impl<typename T::TABLE_ROWS>::eval(StreamOffsetSize);
	}

	template <typename T>
	typename T::instance read(uint8_t StreamOffsetSize, bsw::istream_wrapper_c& is)
	{
		using row = typename T::TABLE_ROWS;
		using instance = typename T::instance;
		instance x;
		detail::inst_reader_impl <row, instance, 0, 0 < row::size() >::read(x, StreamOffsetSize, is);
		return x;
	}

	// =====================================================================================
	template <uint16_t ECMA_ID, typename System>
	struct select_type_by_id;

	template <uint16_t ECMA_ID, typename Head>
	struct select_type_by_id <ECMA_ID, typelist::tlist<Head>>
	{
		using value = typename std::conditional <ECMA_ID == Head::ECMA_ID,
				Head,
				typelist::NullType>::type;
	};

	template <uint16_t ECMA_ID, typename Head, typename... Tail>
	struct select_type_by_id <ECMA_ID, typelist::tlist<Head, Tail...>>
	{
		using value = typename std::conditional <ECMA_ID == Head::ECMA_ID,
				Head,
				typename select_type_by_id<ECMA_ID, typelist::tlist<Tail...>>::value>::type;
	};

	template <typename T>
	struct generic_memory_size
	{
		static std::size_t eval(uint8_t StreamOffsetSize)
		{
			return memory_size<T>(StreamOffsetSize);
		}
	};

	template <>
	struct generic_memory_size<typelist::NullType>
	{
		static std::size_t eval(uint8_t StreamOffsetSize)
		{
			return 0;
		}
	};

	template <typename Ts, size_t I>
	struct memory_size_at_impl
	{
		static std::size_t visit(size_t idx, uint8_t StreamOffsetSize)
		{
			if (idx == I - 1)
			{
				return generic_memory_size<typename select_type_by_id <I-1, Ts>::value>::eval(StreamOffsetSize);
			}
			else
			{
				return memory_size_at_impl<Ts, I - 1>::visit(idx, StreamOffsetSize);
			}
		}
	};

	template <typename Ts>
	struct memory_size_at_impl<Ts, 0>
	{
		static std::size_t visit(size_t idx, uint8_t StreamOffsetSize) { return 0; }
	};

	template <typename Ts>
	std::size_t memory_size_at(size_t idx, uint8_t StreamOffsetSize)
	{
		return memory_size_at_impl<Ts, Ts::size()>::visit(idx, StreamOffsetSize);
	}
} // ns pefile

#define CLR_TABLES_SYSTEM_BEGIN(PFX)				\
	struct PFX										\
	{												\
		typedef typelist::tlist<>	

#define CLR_TABLE_DEF_BEGIN(NAME, IDVAL)			\
	CONCATENATE (tables_pre_, NAME);				\
	struct NAME										\
	{												\
		static constexpr uint16_t ECMA_ID = IDVAL;	\
		static constexpr const char* name () {		\
			return STRINGIZE(NAME);					\
		}											\
		typedef typelist::tlist<>

#define CLR_ROW(TYPE, NAME)									\
	CONCATENATE (rows_pre_, NAME);							\
	struct NAME												\
	{														\
		using type = TYPE;									\
		static constexpr const char* name ()				\
		{													\
			return STRINGIZE(NAME);							\
		}													\
		static constexpr std::size_t impl_index ()			\
		{													\
			return CONCATENATE (rows_pre_, NAME)::size ();	\
		}													\
	};														\
	typedef typelist::tlist_push_back <NAME, CONCATENATE(rows_pre_, NAME)>::type

#define CLR_TABLE_DEF_END(NAME)									\
	TABLE_ROWS;													\
	typedef detail::tuple_type_cnv<TABLE_ROWS>::type instance;	\
	};															\
	typedef typelist::tlist_push_back <NAME, CONCATENATE(tables_pre_, NAME)>::type

#define CLR_TABLES_SYSTEM_END									\
	TABLES;														\
	}

#define CLR_TYPE_BY_ID(SYSTEM, ECMA_ID)	select_type_by_id <ECMA_ID, SYSTEM::TABLES>::value

#endif

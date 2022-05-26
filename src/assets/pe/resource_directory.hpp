#ifndef __PEFILE_RESOURCE_DIRECTORY_HPP__
#define __PEFILE_RESOURCE_DIRECTORY_HPP__

#include <string>
#include <map>

namespace pefile
{
	static constexpr int RCT_NEWRESOURCE = 0x2000;
	enum resource_type_t
	{
		CURSOR = 1,
		BITMAP = 2,
		ICON = 3,
		MENU = 4,
		DIALOG = 5,
		STRING = 6,
		FONTDIR = 7,
		FONT = 8,
		ACCELERATORS = 9,
		RCDATA = 10,
		MESSAGETABLE = 11,
		GROUP_CURSOR = 12,
		GROUP_ICON = 14,
		VERSION = 16,
		NEWBITMAP = (BITMAP | RCT_NEWRESOURCE),
		NEWMENU = (MENU | RCT_NEWRESOURCE),
		NEWDIALOG = (DIALOG | RCT_NEWRESOURCE),
		DLGINCLUDE = 17,
		PLUGPLAY = 19,
		VXD = 20,
		ANICURSOR = 21,
		ANIICON = 22,
		HTML = 23,
		MANIFEST = 24
	};

	class file_c;

	class resource_name_c
	{
		friend bool operator < (const resource_name_c& a, const resource_name_c& b);
	public:
		resource_name_c() = default;
		resource_name_c(int id);
		resource_name_c(const std::wstring& name);

		bool is_id() const;
		int id() const;
		void id(int x);

		std::wstring name() const;
		void name(const std::wstring& x);

		bool is_special() const;

	
	private:
		std::pair <int, std::wstring> m_value;
	};
	// =============================================================================
	bool operator < (const resource_name_c& a, const resource_name_c& b);
	std::wostream& operator << (std::wostream& os, const resource_name_c& a);
	std::ostream& operator << (std::ostream& os, const resource_name_c& a);
	// =============================================================================
	class resource_c
	{
	public:
		resource_c();

		int language_code() const;
		void language_code(int x);

		uint32_t offset() const;
		void offset(uint32_t x);

		uint32_t size() const;
		void size(uint32_t x);

		const resource_name_c& name() const;
		void name(resource_name_c& rn);

		std::size_t offset_in_file(const file_c& f) const;

	private:
		int      m_language_code;
		uint32_t m_offset;
		uint32_t m_size;
		resource_name_c m_rn;
	};
	// =============================================================================
	namespace detail
	{
		class resource_dir_builder_c;
	}
	class resource_dir_c
	{
		friend class detail::resource_dir_builder_c;
	public:
		using second_level_t = std::multimap <resource_name_c, resource_c>;
		using first_level_t  = std::map      <resource_name_c, second_level_t>;
		using iterator = second_level_t::const_iterator;
	public:
		class names_iterator_c
		{
			friend class resource_dir_c;
		public:
			names_iterator_c& operator++ ();
			bool operator == (const names_iterator_c& a);
			bool operator != (const names_iterator_c& a);
			const resource_name_c* operator -> () const;
			const resource_name_c& operator * () const;
		private:
			using itr_t = resource_dir_c::first_level_t::const_iterator;
		private:
			names_iterator_c(itr_t i);
		private:
			itr_t m_itr;
		};
	public:
		iterator begin(const resource_name_c& rn) const;
		iterator end(const resource_name_c& rn) const;
		bool exists(int id) const;

		names_iterator_c names_begin() const;
		names_iterator_c names_end() const;
	protected:
		first_level_t m_dir;
	};

} // ns pefile

#endif

#include <system.h>
#include <config.h>
#include <header.h>
#include <map>

#pragma once

namespace ink::runtime::internal
{
struct ref_block;
} // namespace ink::runtime::internal

namespace ink {
namespace decompiler {

class binstr_modifier {
public:
#ifdef INK_ENABLE_STL
	binstr_modifier(const char* filename);
#endif
	// Create story from allocated binary data in memory. If manage is true, this class will delete
	//  the pointers on destruction
	binstr_modifier(unsigned char* binary, size_t len, bool manage = true);
	virtual ~                       binstr_modifier();
	std::map<uint32_t, std::string> get_string_table() const;
	size_t                          modify_string_table(
		                           const std::map<std::string, std::string>& orig_to_replace, std::ostream& buffer
		                       ) const;
	inline const ip_t end() const { return _file + _length; }
private:
	void setup_pointers();

	// file information
	unsigned char* _file;
	size_t _length;

	ink::internal::header  _header;

	// string table
	const char* _string_table;
	size_t _string_table_size;

	const char* _list_meta;
	const list_flag* _lists;

	// container info
	uint32_t* _container_list;
	uint32_t _container_list_size;
	uint32_t _num_containers;

	// container hashes
	hash_t* _container_hash_start;
	hash_t* _container_hash_end;

	// instruction info
	ip_t _instruction_data;

	// story block used to creat various weak pointers
	runtime::internal::ref_block* _block;

	// whether we need to delete our binary data after we destruct
	bool _managed;

};

} // decompiler
} // ink
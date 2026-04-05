#include "html.h"
#include "string_id.h"
#include <cassert>
#include <unordered_map>

#ifndef LITEHTML_NO_THREADS
	#include <mutex>
	static std::mutex mutex;
	#define lock_guard std::lock_guard<std::mutex> lock(mutex)
#else
	#define lock_guard
#endif

namespace litehtml
{

// Using function-local statics (Meyer's singleton) avoids double-free at exit:
// file-scope statics in static libraries can be initialized/destroyed multiple
// times when the translation unit is linked into more than one DSO, or when
// LTO merges COMDATs unexpectedly.  Function-local statics are guaranteed to
// be initialized exactly once (C++11 §6.7) and destroyed in reverse order of
// construction at program exit — after all TU-scope statics.
//
// NOTE on eviction: string_id is an integer index into get_array(). Entries
// cannot be removed without invalidating all live string_id values held by
// every element/selector/property across all open documents. The vocabulary
// is bounded by the app's CSS and HTML content — not by number of document
// loads — so growth is expected to plateau quickly (few hundred entries).
// The unordered_map gives O(1) lookups which keeps individual calls cheap
// even as the table grows.
static std::unordered_map<string, string_id>& get_map() {
    static std::unordered_map<string, string_id> map;
    return map;
}
static std::vector<string>& get_array() {
    static std::vector<string> array;
    return array;
}

static int init()
{
	// Pre-reserve for initial_string_ids + headroom for app CSS class names.
	// Avoids rehash churn during the first few document loads.
	auto& map   = get_map();
	auto& array = get_array();
	map.reserve(512);
	array.reserve(512);

	string_vector names;
	split_string(initial_string_ids, names, ",");
	for (auto& name : names)
	{
		trim(name);
		assert(name[0] == '_' && name.back() == '_');
		name = name.substr(1, name.size() - 2);				// _border_color_ -> border_color
		std::replace(name.begin(), name.end(), '_', '-');	// border_color   -> border-color
		_id(name);  // this will create association _border_color_ <-> "border-color"
	}
	return 0;
}
static int dummy = init();

const string_id empty_id = _id("");
const string_id star_id = _id("*");

string_id _id(const string& str)
{
	lock_guard;
	auto& map = get_map();
	auto& array = get_array();
	auto it = map.find(str);
	if (it != map.end()) return it->second;
	// else: str not found, add it to the array and the map
	array.push_back(str);
	return map[str] = (string_id)(array.size() - 1);
}

const string& _s(string_id id)
{
	lock_guard;
	return get_array()[id];
}

} // namespace litehtml
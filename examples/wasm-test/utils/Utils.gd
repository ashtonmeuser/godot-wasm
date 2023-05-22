extends Object
class_name Utils

static func comparable(o):
	return o.hash() if ((o is Dictionary or o is Object) and o != null) else o

static func make_regex(pattern: String) -> RegEx:
	var regex = RegEx.new()
	var error = regex.compile(pattern)
	assert(error == OK, "Invalid regex pattern: %s" % pattern)
	return regex

static func file_length(f: File) -> int:
	return f.get_len()

static func to_utf8(s: String) -> PoolByteArray:
	return s.to_utf8()

static func godot_4() -> bool:
	return Engine.get_version_info()["major"] == 4

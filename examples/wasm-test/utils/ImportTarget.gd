extends Object
class_name ImportTarget

# Dummy import to supply to Wasm modules
static func _dummy_import(a = "", b = "", c = "", d = ""):
	var message = "Dummy import %s %s %s %s" % [a, b, c, d]
	print(message.strip_edges())
	return a

func dummy_import() -> Array:
	return [self, "_dummy_import"]

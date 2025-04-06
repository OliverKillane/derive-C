import gdb

DERIVE_C_PREFIX = "derive_c_"
DERIVE_C_LIB = "Derive-C"

class VectorPrinter:
    """Pretty Printer for a generated vector-like structure."""

    def __init__(self, val):
        self.val = val
        self.capacity = self.val['capacity']
        self.size = self.val['size']

    def to_string(self):
        return f"Vector {{ capacity = {self.val['capacity']}, size = {self.val['size']} }}"

    def display_hint(self):
        return "array"


class HashMapPrinter:
    """Pretty Printer for a generated hashmap-like structure."""

    def __init__(self, val):
        self.val = val
        self.self_type = self.val.type.target()
        self.capacity = int(val["capacity"])
        self.items = int(val["items"])
        self.key_type = val["keys"].type.target()["key"].type  # Get the type of KEY_ENTRY
        self.value_type = val["values"].type.target()  # Get the type of V

    def to_string(self):
        def entries():
            undefined_entry = 0
            for i in range(self.capacity):    
                key_entry = self.val["keys"][i]
                value_entry = self.val["values"][i]
                
                if int(key_entry["present"]):
                    if undefined_entry < i:
                        yield f"<undefined from {undefined_entry} to {i}>"
                    key = key_entry["key"]
                    value = value_entry
                    yield f"    [{key}] => [{value}],"
                    undefined_entry = i + 1
            
            
        items = "\n".join(entries())
        return f"""{DERIVE_C_LIB} HashMap<Key={self.key_type}, Value={self.value_type}> as {self.self_type} {{
    items = {self.items},
    capacity = {self.capacity},
}} with items {{
{items}    
}}"""

    def display_hint(self):
        return "map"



class ArenaPrinter:
    def __init__(self, val): pass
    def to_string(self): pass
    def children(self): pass

PRINTERS = {
    "vector": VectorPrinter,
    "hashmap": HashMapPrinter,
    "arena": ArenaPrinter
}

def lookup_function(val):
    """Detects a struct with a field starting with 'derive_c_' and applies the corresponding printer."""
    try:
        for field in val.type.strip_typedefs().target().fields():
            if field.name and field.name.startswith(DERIVE_C_PREFIX):
                derived_type = field.name[len(DERIVE_C_PREFIX):]
                return PRINTERS.get(derived_type, None)(val) if derived_type in PRINTERS else None
    except:
        pass
    return None

gdb.pretty_printers.append(lookup_function)

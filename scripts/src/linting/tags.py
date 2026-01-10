# Tags for marking template usage
DERIVE_C_TAG: str = "[DERIVE-C]"
FOR_TEMPLATE: str = f"// {DERIVE_C_TAG} for template"
FOR_INPUT_ARG: str = f"// {DERIVE_C_TAG} for input arg"

# For undefining arguments
ARGUMENT = f"// {DERIVE_C_TAG} argument"

# Tags for marking includes
EXPORT_COMMENT: str = r"// IWYU pragma: export"
STDLIB_INCLUDES: str = rf"// {DERIVE_C_TAG} stdlib includes"
DERIVE_C_INCLUDES: str = rf"// {DERIVE_C_TAG} lib includes"
USED_TEMPLATE_INCLUDES: str = rf"// {DERIVE_C_TAG} used template includes"
CONTAINER_SPECIFIC_INCLUDES: str = rf"// {DERIVE_C_TAG} container includes"

# Markers for exported macros
MACRO_PREFIX: str = "DC_"
MACRO_FIX_PRIVATE: str = "_DC_"

set(CPPFLAGS -DHAVE_CONFIG_H)

set(COMPONENT_SRCDIRS nfc nfc/buses nfc/chips nfc/drivers)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_PRIV_INCLUDEDIRS nfc nfc/buses nfc/chips nfc/drivers)

set(COMPONENT_REQUIRES main)

register_component()

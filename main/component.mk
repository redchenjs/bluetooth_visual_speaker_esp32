#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

COMPONENT_SRCDIRS := src src/tasks src/system src/driver src/device
COMPONENT_ADD_INCLUDEDIRS := inc
COMPONENT_EMBED_FILES := res/snd/snd0.mp3 res/snd/snd1.mp3 res/ani/ani0.gif res/ani/ani1.gif res/ani/ani2.gif res/ani/ani3.gif

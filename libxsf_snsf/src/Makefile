UNAME:=	$(shell uname -s)

SRCDIR:=	$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

FRAMEWORK_SRCS:=	$(sort $(wildcard $(SRCDIR)in_xsf_framework/*.cpp))
FRAMEWORK_OBJS:=	$(subst $(SRCDIR),,$(FRAMEWORK_SRCS:%.cpp=%.o))

in_2sf_SRCS:=	$(wildcard $(SRCDIR)in_2sf/*.cpp) $(wildcard $(SRCDIR)in_2sf/desmume/*.cpp) $(wildcard $(SRCDIR)in_2sf/desmume/addons/*.cpp) \
	$(wildcard $(SRCDIR)in_2sf/desmume/metaspu/*.cpp) $(wildcard $(SRCDIR)in_2sf/desmume/metaspu/SoundTouch/*.cpp) \
	$(wildcard $(SRCDIR)in_2sf/desmume/utils/*.cpp) $(wildcard $(SRCDIR)in_2sf/desmume/utils/AsmJit/*.cpp) \
	$(wildcard $(SRCDIR)in_2sf/desmume/utils/AsmJit/base/*.cpp) $(wildcard $(SRCDIR)in_2sf/desmume/utils/AsmJit/x86/*.cpp)
in_2sf_SRCS:=	$(filter-out $(SRCDIR)in_2sf/desmume/debug.cpp $(SRCDIR)in_2sf/desmume/metaspu/SoundTouch/cpu_detect_x86_win.cpp,$(in_2sf_SRCS))
in_gsf_SRCS:=	$(wildcard $(SRCDIR)in_gsf/*.cpp) $(wildcard $(SRCDIR)in_gsf/vbam/apu/*.cpp) $(wildcard $(SRCDIR)in_gsf/vbam/gba/*.cpp)
in_ncsf_SRCS:=	$(wildcard $(SRCDIR)in_ncsf/*.cpp) $(wildcard $(SRCDIR)in_ncsf/SSEQPlayer/*.cpp)
in_snsf_SRCS:=	$(wildcard $(SRCDIR)in_snsf/*.cpp) $(wildcard $(SRCDIR)in_snsf/snes9x/*.cpp) $(wildcard $(SRCDIR)in_snsf/snes9x/apu/*.cpp)

DLLS=	in_2sf in_gsf in_ncsf in_snsf
DLLS:=	$(sort $(addsuffix .dll,$(DLLS)))

COMPILER:=	$(shell $(CXX) -v 2>/dev/stdout)

MY_CPPFLAGS=	$(CPPFLAGS) -std=gnu++11 -I$(SRCDIR)in_xsf_framework -I/g/Code/Winamp\ SDK -DWINAMP_PLUGIN -DUNICODE_INPUT_PLUGIN
MY_CXXFLAGS=	$(CXXFLAGS) -std=gnu++11 -pipe -Wall -Wctor-dtor-privacy -Wold-style-cast -Wextra -Wno-div-by-zero -Wfloat-equal -Wshadow -Winit-self -Wcast-qual -Wunreachable-code -Wabi -Woverloaded-virtual -Wno-long-long -Wno-switch -I$(SRCDIR)in_xsf_framework -I/g/Code/Winamp\ SDK -DWINAMP_PLUGIN -DUNICODE_INPUT_PLUGIN
ifeq (,$(findstring clang,$(COMPILER)))
MY_CXXFLAGS+=	-Wlogical-op
endif
MY_LDFLAGS=	$(LDFLAGS) -lz

DLL_SRCS_template=	$(1)_SRCS:=	$$(sort $$($(1)_SRCS))
DLL_OBJS_template=	$(1)_OBJS:=	$$(subst $(SRCDIR),,$$($(1)_SRCS:%.cpp=%.o))

$(foreach dll,$(DLLS),$(eval $(call DLL_SRCS_template,$(basename $(notdir $(dll))))))
$(foreach dll,$(DLLS),$(eval $(call DLL_OBJS_template,$(basename $(notdir $(dll))))))

SRCS:=	$(sort $(FRAMEWORK_SRCS) $(foreach dll,$(DLLS),$($(basename $(notdir $(dll)))_SRCS)))
OBJS:=	$(sort $(FRAMEWORK_OBJS) $(foreach dll,$(DLLS),$($(basename $(notdir $(dll)))_OBJS)))
DEPS:=	$(OBJS:%.o=%.d)

.PHONY: all debug clean

.SUFFIXES:
.SUFFIXES: .cpp .o .d .a .dll

all: $(DLLS)
debug: MY_CXXFLAGS+=	-g -D_DEBUG
debug: all

define DLL_template
$(1): $$(FRAMEWORK_OBJS) $$($$(basename $$(notdir $(1)))_OBJS)
	@echo "Linking $$@..."
	@$$(CXX) -shared -fPIC $$(MY_CXXFLAGS) -o $$@ $$^ $$(MY_LDFLAGS)
endef
define SRC_template
$$(subst $(SRCDIR),,$(1:%.cpp=%.o)): $(1)
	@echo "Compiling $$<..."
	@$$(CXX) $$(MY_CXXFLAGS) -o $$(subst $(SRCDIR),,$$@) -c $$<
endef
define DEP_template
$$(subst $(SRCDIR),,$(1:%.cpp=%.d)): $(1)
	@echo "Calculating depends for $$<..."
	-@mkdir -p $$(@D)
	@$$(CXX) $$(MY_CPPFLAGS) -MM -MF $$(subst $(SRCDIR),,$$@).tmp $$<
	@sed 's,$$(notdir $$*)\.o[ :]*,$$(subst /,\/,$$*).o $$(subst /,\/,$$@): ,g' < $$(subst $(SRCDIR),,$$@).tmp > $$(subst $(SRCDIR),,$$@)
	@rm $$(subst $(SRCDIR),,$$@).tmp
endef

$(foreach dll,$(DLLS),$(eval $(call DLL_template,$(dll))))
$(foreach src,$(SRCS),$(eval $(call SRC_template,$(src))))
$(foreach src,$(SRCS),$(eval $(call DEP_template,$(src))))

$(subst $(SRCDIR),,$(in_2sf_SRCS:%.cpp=%.d)): MY_CPPFLAGS+=	-msse
$(in_2sf_OBJS): MY_CXXFLAGS+=	-msse -DHAVE_LIBZ

in_2sf/desmume/MMU.o: MY_CXXFLAGS+=	-Wno-shadow
in_2sf/desmume/utils/AsmJit/base/codegen.o: MY_CXXFLAGS+=	-Wno-unused-value
in_2sf/desmume/utils/AsmJit/base/runtime.o: MY_CXXFLAGS+=	-Wno-unused-parameter
in_2sf/desmume/utils/AsmJit/base/vmem.o: MY_CXXFLAGS+=	-Wno-missing-field-initializers -Wno-unused-function
ifneq (,$(findstring clang,$(COMPILER)))
in_2sf/desmume/utils/AsmJit/base/vmem.o: MY_CXXFLAGS+=	-Wno-missing-braces
endif
in_2sf/desmume/utils/AsmJit/x86/x86assembler.o: MY_CXXFLAGS+=	-Wno-unused-variable
ifneq (,$(findstring clang,$(COMPILER)))
in_2sf/desmume/utils/AsmJit/x86/x86assembler.o: MY_CXXFLAGS+=	-Wno-missing-braces
endif
in_2sf/desmume/utils/AsmJit/x86/x86context.o: MY_CXXFLAGS+=	-Wno-unused-parameter -Wno-unused-value -Wno-unused-variable
in_gsf/vbam/gba/GBA-arm.o: MY_CXXFLAGS+=	-Wno-unused-variable
ifeq (,$(findstring clang,$(COMPILER)))
in_gsf/vbam/gba/GBA-arm.o: MY_CXXFLAGS+=	-Wno-unused-but-set-variable
endif

clean:
	@echo "Cleaning OBJs and DLLs..."
	-@rm $(OBJS) $(DLLS)

-include $(DEPS)

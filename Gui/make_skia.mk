# Defines found in BUILD.gn
# SK_R32_SHIFT=16 seems to be assumed on Linux and Fuschsia
# SK_ASSUME_GL_ES is to assume GLES and not GL (SK_ASSUME_GL=1)
# SK_GAMMA_APPLY_TO_A8 seems to be set for "skia_private" config.
# GR_OP_ALLOCATE_USE_NEW seems to be enable if we have gpu support
# SKIA_IMPLEMENTATION=1 seems to be needed when building the library
# SK_UNICODE_AVAILABLE=1 for unicode support in text shaping modules
# SK_SHAPER_HARFBUZZ_AVAILABLE=1 for harfbuzz support
DEFINES := SK_R32_SHIFT=16 SK_ASSUME_GL_ES=1 SK_GAMMA_APPLY_TO_A8 GR_OP_ALLOCATE_USE_NEW SKIA_IMPLEMENTATION=1 SK_SUPPORT_GPU=1 SK_GL=1 SK_GANESH=1 SK_DISABLE_LEGACY_PNG_WRITEBUFFER=1

GCC_VERSION := $(shell [ $$(g++ -dumpversion) -gt 10 ] || echo old)
ifeq ($(GCC_VERSION),old)
# Compatibility on older GCC:
# SK_CPU_SSE_LEVEL -- disable SSE things that fail to compile.
# JUMPER_IS_SCALAR -- to disable SSE intrinsics for older GCC that don't support it.
# SK_OLD_COMPILER -- custom flag that disables some unsupported intrinsics.
DEFINES := $(DEFINES) SK_OLD_COMPILER SK_CPU_SSE_LEVEL=0 JUMPER_IS_SCALAR
endif

# These are only needed if SkShaper and/or SkParagraph are used.
#DEFINES := $(DEFINES) SK_UNICODE_AVAILABLE=1 SK_SHAPER_HARFBUZZ_AVAILABLE=1

# There seems to be a number of options for enabling SSE and AVX. I don't think that is needed since
# we will be using it through OpenGL.

# Note: The below issue is fixed now, and therefore we can use -O3 freely again. This issue broke Skia even on -O2.
# Note: Using -O3 with GCC seems to break the initialization of the ComponentArray passed to Swizzle
# in SkSLIRGenerator.cpp, in the function getNormalizeSkPositionCode. Don't know if this is UB in
# the Skia code or a micompilation by GCC. Should probably be reported somehow...

# The -Wno-psabi flag is to silence a warning about ABI compatibility since we don't explicitly enable AVX at the time.

# The cheap flags are used to build the standalone SKSL compiler to reduce compile times a bit.
CHEAP_CXXFLAGS := -std=c++17 -fPIC -iquote. -Wno-psabi -I/usr/include/freetype2 $(addprefix -D,$(DEFINES))
# These are the flags for the final library. We want to use -O3 here for speed.
CXXFLAGS := -O3 $(CHEAP_CXXFLAGS)

# Workdir, where to store build files. We increase this when Skia is updated to avoid build failures, since
# the makefile is not perfect at tracking dependencies.
WORKDIR := out/v2

# Note: We skipped these: codec
SKIA_LIBS :=

# To use the SkParagraph library (requires more run-time and possibly compile-time dependencies):
#SKIA_LIBS := skshaper skparagraph

SOURCE_DIRS := base core effects effects/colorfilters effects/imagefilters fonts gpu gpu/ganesh gpu/ganesh/effects gpu/ganesh/geometry gpu/ganesh/gl gpu/ganesh/gl/builders gpu/ganesh/gl/egl gpu/ganesh/gl/glx gpu/ganesh/glsl gpu/ganesh/gradients gpu/ganesh/image gpu/ganesh/mock gpu/ganesh/ops gpu/ganesh/surface gpu/ganesh/tessellate gpu/ganesh/text gpu/tessellate image lazy opts pathops shaders shaders/gradients sksl sksl/ir sksl/codegen sksl/lex sksl/tracing sksl/transform sksl/analysis text sfnt text/gpu utils
PORTS := SkDebug_stdio.cpp SkDiscardableMemory_none.cpp SkFontConfigInterface*.cpp SkFontMgr_fontconfig*.cpp SkFontMgr_FontConfigInterface*.cpp SkFontHost_*.cpp SkGlobalInitialization_default.cpp SkMemory_malloc.cpp SkOSFile_posix.cpp SkOSFile_stdio.cpp SkOSLibrary_posix.cpp SkImageGenerator_none.cpp
CODEC := SkCodec.cpp SkPixmapUtils.cpp SkSampler.cpp SkCodecImageGenerator.cpp SkImageGenerator_FromEncoded.cpp
SOURCES := $(wildcard $(addsuffix /*.cpp,$(addprefix src/,$(SOURCE_DIRS)))) $(wildcard $(addprefix src/ports/,$(PORTS))) $(wildcard $(addprefix src/codec/,$(CODEC)))
SOURCES := $(filter-out src/sksl/SkSLMain.cpp,$(SOURCES)) # Remove the main file...
SOURCES := $(filter-out src/gpu/gl/GrGLMakeNativeInterface_none.cpp,$(SOURCES))
SOURCES := $(filter-out src/gpu/GrPathRendering_none.cpp,$(SOURCES))
LIB_SOURCES := $(wildcard $(addsuffix /src/*.cpp,$(addprefix modules/,$(SKIA_LIBS))))
LIB_SOURCES := $(filter-out %_coretext.cpp,$(LIB_SOURCES))
OBJECTS := $(patsubst src/%.cpp,$(WORKDIR)/%.o,$(SOURCES))
LIB_OBJECTS := $(patsubst modules/%.cpp,$(WORKDIR)/modules/%.o,$(LIB_SOURCES))

SKSL_SRC := src/core/SkMalloc.cpp src/core/SkMath.cpp src/core/SkSemaphore.cpp src/core/SkThreadID.cpp src/gpu/GrBlockAllocator.cpp src/gpu/GrMemoryPool.cpp src/ports/SkMemory_malloc.cpp $(wildcard src/sksl/*.cpp src/sksl/ir/*.cpp)
SKSL_OBJ := $(patsubst src/%.cpp,$(WORKDIR)/slc/%.o,$(SKSL_SRC))
SKSL_PRECOMP := $(addsuffix .sksl,$(addprefix src/sksl/sksl_,fp frag geom gpu interp pipeline vert))
SKSL_DEHYDRATED := $(patsubst src/sksl/%.sksl,src/sksl/generated/%.dehydrated.sksl,$(SKSL_PRECOMP))

SKCMS_FILES := modules/skcms/skcms.cc $(wildcard modules/skcms/src/*.cc)
SKCMS_OBJECTS := $(patsubst modules/skcms/%.cc,$(WORKDIR)/skcms/%.o,$(SKCMS_FILES))

ifeq ($(OUTPUT),)
OUTPUT := skia.a
endif

.PHONY: all
all: $(OUTPUT)

.PHONY: clean
clean:
	rm -r $(WORKDIR)/

$(OUTPUT): $(OBJECTS) $(LIB_OBJECTS) $(SKCMS_OBJECTS)
	@echo "Linking $(OUTPUT)..."
	@rm -f $(OUTPUT)
	@ar rcs $(OUTPUT) $(OBJECTS) $(LIB_OBJECTS) $(SKCMS_OBJECTS)

$(OBJECTS): $(WORKDIR)/%.o: src/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	g++ -c $(CXXFLAGS) -o $@ $<

$(LIB_OBJECTS): $(WORKDIR)/modules/%.o: modules/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	g++ -c $(CXXFLAGS) -I/usr/include/harfbuzz/ -o $@ $<

$(SKCMS_OBJECTS): $(WORKDIR)/skcms/%.o: modules/skcms/%.cc
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	g++ -DSKCMS_DISABLE_HSW -DSKCMS_DISABLE_SKX -c -I include/modules/skcms -O3 -fPIC $< -o $@

# The rules below compile the SKSL sources.

# Uncommenting this line causes us to re-compile the hydrated SKSL files that are included in the build.
# They are actually committed to the repository, so we don't need to do that, especially as it increases
# built time by quite a lot since we need to compile parts of Skia multiple times.

# $(WORKDIR)/sksl/SkSLCompiler.o: src/sksl/SkSLCompiler.cpp $(SKSL_DEHYDRATED)

$(WORKDIR)/skslc: $(SKSL_OBJ)
	g++ -pthread -o $@ $(SKSL_OBJ)

$(SKSL_OBJ): $(WORKDIR)/slc/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ -c $(CHEAP_CXXFLAGS) -DSKSL_STANDALONE -o $@ $<

src/sksl/sksl_fp.sksl: src/sksl/sksl_fp_raw.sksl
	cat include/private/GrSharedEnums.h $^ | sed 's|^#|// #|g' > $@

$(SKSL_DEHYDRATED): src/sksl/generated/%.dehydrated.sksl: src/sksl/%.sksl $(WORKDIR)/skslc
	cd src/sksl/ && ../../$(WORKDIR)/skslc $(notdir $<) generated/$(notdir $@)

print:
	@echo $(SOURCES)
	@echo $(SKSL_DEHYDRATED)

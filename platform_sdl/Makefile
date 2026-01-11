CXX ?= clang++
CXXFLAGS ?= -std=c++17 -I..

# Depends on SDL2 and SDL2_gfx. Install via Homebrew with:
# brew install sdl2 sdl2_gfx

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(strip $(SDL_CFLAGS)),)
  SDL_CFLAGS := -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
  SDL_LIBS := -L/opt/homebrew/lib -lSDL2
endif

SDL_GFX_CFLAGS := $(shell pkg-config --cflags SDL2_gfx 2>/dev/null)
SDL_GFX_LIBS := $(shell pkg-config --libs SDL2_gfx 2>/dev/null)

ifeq ($(strip $(SDL_GFX_CFLAGS)),)
  SDL_GFX_CFLAGS := -I/opt/homebrew/include/SDL2
  SDL_GFX_LIBS := -L/opt/homebrew/lib -lSDL2_gfx
endif

TARGET := miniacid
SOURCES := ../src/dsp/mini_tb303.cpp ../src/dsp/mini_drumvoices.cpp ../src/dsp/tube_distortion.cpp ../src/dsp/miniacid_engine.cpp ../src/ui/miniacid_display.cpp ../src/ui/pages/help_page.cpp ../src/ui/pages/help_dialog.cpp ../src/ui/pages/tb303_params_page.cpp ../src/ui/pages/waveform_page.cpp ../src/ui/pages/pattern_edit_page.cpp ../src/ui/pages/drum_sequencer_page.cpp ../src/ui/pages/song_page.cpp ../src/ui/pages/project_page.cpp ../src/ui/components/pattern_selection_bar.cpp ../src/ui/components/bank_selection_bar.cpp ../src/ui/components/label_option.cpp ../src/audio/desktop_audio_recorder.cpp ../src/audio/wasm_audio_recorder.cpp ../cardputer_display.cpp ../scenes.cpp ../json_evented.cpp sdl_main.cpp sdl_display.cpp scene_storage_sdl.cpp ../src/ui/ui_core.cpp

ROOT := $(abspath ..)
DOCKER ?= docker
EMCC_IMAGE ?= emscripten/emsdk
WASM_FLAGS := -I.. -std=c++17 -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=262144 -sASSERTIONS=1 -sUSE_SDL_GFX=2 -sEXPORTED_RUNTIME_METHODS='[UTF8ToString,stringToUTF8,lengthBytesUTF8,HEAPU8]'

APP_BUNDLE := miniacid.app
SDL2_DYLIB := /opt/homebrew/opt/sdl2/lib/libSDL2-2.0.0.dylib
SDL2_GFX_DYLIB := /opt/homebrew/opt/sdl2_gfx/lib/libSDL2_gfx-1.0.0.dylib

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SDL_GFX_CFLAGS) $^ $(SDL_LIBS) $(SDL_GFX_LIBS) -o $@

wasm: $(SOURCES)
	mkdir -p $(ROOT)/web
	$(DOCKER) run --rm -v $(ROOT):/src -w /src/platform_sdl $(EMCC_IMAGE) emcc $(SOURCES) $(WASM_FLAGS) -o /src/web/miniacid.html

bundle: $(TARGET)
	@echo "Creating app bundle..."
	mkdir -p $(APP_BUNDLE)/Contents/MacOS
	mkdir -p $(APP_BUNDLE)/Contents/Frameworks
	mkdir -p $(APP_BUNDLE)/Contents/Resources
	cp $(TARGET) $(APP_BUNDLE)/Contents/MacOS/
	cp $(SDL2_DYLIB) $(APP_BUNDLE)/Contents/Frameworks/
	cp $(SDL2_GFX_DYLIB) $(APP_BUNDLE)/Contents/Frameworks/
	install_name_tool -change $(SDL2_DYLIB) @executable_path/../Frameworks/libSDL2-2.0.0.dylib $(APP_BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(SDL2_GFX_DYLIB) @executable_path/../Frameworks/libSDL2_gfx-1.0.0.dylib $(APP_BUNDLE)/Contents/MacOS/$(TARGET)
	install_name_tool -change $(SDL2_DYLIB) @executable_path/../Frameworks/libSDL2-2.0.0.dylib $(APP_BUNDLE)/Contents/Frameworks/libSDL2_gfx-1.0.0.dylib
	@echo "Code signing libraries and executable..."
	codesign --force --deep --sign - $(APP_BUNDLE)/Contents/Frameworks/libSDL2-2.0.0.dylib
	codesign --force --deep --sign - $(APP_BUNDLE)/Contents/Frameworks/libSDL2_gfx-1.0.0.dylib
	codesign --force --deep --sign - $(APP_BUNDLE)/Contents/MacOS/$(TARGET)
	codesign --force --deep --sign - $(APP_BUNDLE)
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $(APP_BUNDLE)/Contents/Info.plist
	@echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<plist version="1.0"><dict>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<key>CFBundleExecutable</key><string>$(TARGET)</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<key>CFBundleIdentifier</key><string>com.miniacid.app</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<key>CFBundleName</key><string>MiniAcid</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<key>CFBundleVersion</key><string>1.0</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<key>CFBundlePackageType</key><string>APPL</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '</dict></plist>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo "App bundle created at $(APP_BUNDLE)"
	@echo "You can now run: open $(APP_BUNDLE)"

clean:
	rm -f $(TARGET)
	rm -rf $(APP_BUNDLE)

.PHONY: all clean wasm bundle

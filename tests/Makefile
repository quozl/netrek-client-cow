# tests/Makefile — build and run SDL2 backend unit tests
# Run with: make check
# Requires: pkg-config, SDL2, SDL2_image, SDL2_ttf

TESTS = test_event_pipe test_event_translation test_drawing \
        test_window_mgmt test_sprite test_text test_render_png

SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer 2>/dev/null)
SDL2_LIBS   := $(shell pkg-config --libs   sdl2 SDL2_image SDL2_ttf SDL2_mixer 2>/dev/null)

CFLAGS = -I.. -I../sdl2 -Iunity $(SDL2_CFLAGS) -g -Wall -DHAVE_SDL2=1 -DUNIT_TEST -include ../config.h
LIBS   = $(SDL2_LIBS) -lm

# Common source files compiled into every test binary
COMMON_SRCS = unity/unity.c stubs.c ../sdl2/sdl2window.c ../sdl2/sdl2sprite.c

all: $(TESTS)

test_event_pipe: test_event_pipe.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_event_translation: test_event_translation.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_drawing: test_drawing.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_window_mgmt: test_window_mgmt.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_sprite: test_sprite.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_text: test_text.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

test_render_png: test_render_png.c $(COMMON_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# render-png: build and run the visual render test, output to /tmp/netrek_*.png
render-png: test_render_png
	@echo "Rendering bitmaps to /tmp/netrek_*.png ..."
	@SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy sh -c "cd .. && tests/test_render_png"
	@echo "Open /tmp/netrek_render_test.png, /tmp/netrek_blit_bg_test.png, /tmp/netrek_overlay_test.png"

check: all
	@echo "Running SDL2 backend unit tests (headless)..."
	@PASS=0; FAIL=0; \
	TESTDIR=$$(pwd); \
	for t in $(TESTS); do \
	  if SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
	     sh -c "cd .. && $$TESTDIR/$$t"; then \
	    PASS=$$((PASS+1)); \
	  else \
	    FAIL=$$((FAIL+1)); \
	    echo "FAILED: $$t"; \
	  fi; \
	done; \
	echo "Results: $$PASS passed, $$FAIL failed"; \
	test $$FAIL -eq 0

clean:
	rm -f $(TESTS)

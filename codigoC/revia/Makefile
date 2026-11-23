# REV.IA - DECIFRA.IA
#
#   make          compila o jogo em build/revia  (Linux, macOS, Windows/MSYS2)
#   make run      compila e executa
#   make test     roda os testes de lógica (não precisa da Raylib)
#   make levels   regenera os mapas a partir de tools/gen_levels.py
#   make smoke    executa o jogo "headless" com entradas aleatórias (sem Raylib; usa sanitizers)
#   make clean
#
# A Raylib é encontrada via pkg-config quando possível. Caso contrário, defina
# RAYLIB_DIR=/caminho/da/raylib (com include/ e lib/) ou ajuste LDLIBS abaixo.

CC       ?= cc
CFLAGS   ?= -O2
CFLAGS   += -std=c11 -Wall -Wextra -Wpedantic -Isrc
BUILD    := build
TARGET   := $(BUILD)/revia

SRC      := $(wildcard src/*.c src/*/*.c)
OBJ      := $(SRC:%.c=$(BUILD)/%.o)
DEP      := $(OBJ:.o=.d)

# módulos de lógica, sem dependência de janela/áudio
LOGIC_SRC := src/core/config.c src/player/player.c src/player/physics.c src/world/level.c \
             src/world/collision.c src/entities/entities.c src/narrative/dialogue.c \
             src/narrative/missions.c src/narrative/decisions.c src/education/quiz.c \
             src/education/bias_game.c src/education/codex.c src/systems/progression.c src/systems/save.c

UNAME := $(shell uname -s 2>/dev/null)

ifdef RAYLIB_DIR
  CFLAGS  += -I$(RAYLIB_DIR)/include
  LDFLAGS += -L$(RAYLIB_DIR)/lib
endif

PKG_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)
PKG_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)

ifneq ($(PKG_LIBS),)
  CFLAGS += $(PKG_CFLAGS)
  LDLIBS := $(PKG_LIBS) -lm
else ifneq (,$(findstring MINGW,$(UNAME)))
  LDLIBS := -lraylib -lopengl32 -lgdi32 -lwinmm -lm
else ifneq (,$(findstring MSYS,$(UNAME)))
  LDLIBS := -lraylib -lopengl32 -lgdi32 -lwinmm -lm
else ifeq ($(UNAME),Darwin)
  LDLIBS := -lraylib -framework IOKit -framework Cocoa -framework OpenGL -lm
else
  LDLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

.PHONY: all run test levels smoke clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

# ---- testes sem Raylib ----
$(BUILD)/test_logic: tests/test_logic.c $(LOGIC_SRC)
	@mkdir -p $(BUILD)
	$(CC) -std=c11 -g -Wall -Wextra -Isrc -fsanitize=address,undefined tests/test_logic.c $(LOGIC_SRC) -lm -o $@

test: $(BUILD)/test_logic
	./$(BUILD)/test_logic

levels:
	python3 tools/gen_levels.py

# ---- execução headless com stub da Raylib (apenas verificação) ----
$(BUILD)/smoke: $(SRC) tests/stub/raylib_stub.c
	@mkdir -p $(BUILD)
	$(CC) -std=c11 -g -Wall -Wextra -DREVIA_SMOKE -Isrc -Itests/stub -fsanitize=address,undefined $(SRC) tests/stub/raylib_stub.c -lm -o $@

smoke: $(BUILD)/smoke
	@mkdir -p $(BUILD)/smoke_run && cd $(BUILD)/smoke_run && rm -f revia_save.dat && \
	  ln -sfn ../../levels levels && STUB_MODE=run STUB_FRAMES=6000 ../smoke && \
	  for s in 1 2 3 4 5; do rm -f revia_save.dat; STUB_SEED=$$s STUB_FRAMES=20000 ../smoke || exit 1; done

clean:
	rm -rf $(BUILD)

-include $(DEP)

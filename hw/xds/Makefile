OUTPUT_FILES := libxds.so libxds.a
TEST_FILES := testxds test608 rcwt_dump

CCDATASERVER_PATH ?= ../ccdatabase

ASAN_CFLAGS := -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address
ASAN_LDFLAGS := -lasan
CFLAGS	:= -Wall -g -O0 -fPIC $(ASAN_CFLAGS)
LDFLAGS := $(ASAN_LDFLAGS)
OUT_DIR ?= ./build
TARGET_DIR ?= $(OUT_DIR)

LIBXDS_SRCS := \
	src/xds.c\
	src/xds_608.c\
	src/xds_708.c\

LIBXDS_OBJS := $(patsubst %.c,$(OUT_DIR)/%.o,$(LIBXDS_SRCS))
CFLAGS += -I$(CCDATASERVER_PATH)/build/usr/include

TEST_CFLAGS := -Isrc

TEST608_SRCS := \
	test/test608.c\

TEST608_OBJS := $(patsubst %.c,$(OUT_DIR)/%.o,$(TEST608_SRCS))
TEST608_LDFLAGS := -L$(OUT_DIR) -l:libxds.a -L$(CCDATASERVER_PATH)/build -l:libccdataserver_client.a

RCWT_SRCS := \
	test/rcwt_dump.c\

RCWT_OBJS := $(patsubst %.c,$(OUT_DIR)/%.o,$(RCWT_SRCS))

TESTXDS_SRCS := \
	test/testxds.c\

TESTXDS_OBJS := $(patsubst %.c,$(OUT_DIR)/%.o,$(TESTXDS_SRCS))
TESTXDS_LDFLAGS := -L$(OUT_DIR) -l:libxds.a -L$(CCDATASERVER_PATH)/build -l:libccdataserver_client.a


all: $(OUTPUT_FILES) $(TEST_FILES)

test: $(TEST_FILES)

$(OUT_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

libxds.so: $(LIBXDS_OBJS)
	$(CC) -shared -o $(OUT_DIR)/$@ $^ $(LDFLAGS)

libxds.a: $(LIBXDS_OBJS)
	$(AR) rsc $(OUT_DIR)/$@ $^

$(OUT_DIR)/test/%.o: test/%.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(TEST_CFLAGS)

testxds: $(TESTXDS_OBJS) libxds.a
	$(CC) -o $(OUT_DIR)/$@ $(TESTXDS_OBJS) $(LDFLAGS) $(TESTXDS_LDFLAGS)

test608: $(TEST608_OBJS) libxds.a
	$(CC) -o $(OUT_DIR)/$@ $(TEST608_OBJS) $(LDFLAGS) $(TEST608_LDFLAGS)

rcwt_dump: $(RCWT_OBJS)
	$(CC) -o $(OUT_DIR)/$@ $^ $(LDFLAGS) $(RCWT_LDFLAGS)

install: $(OUTPUT_FILES)

install_test: $(TEST_FILES)
	-install -d $(TARGET_DIR)/usr/bin
	-install -m 0755 $(OUT_DIR)/testxds $(TARGET_DIR)/usr/bin
	-install -m 0755 $(OUT_DIR)/test608 $(TARGET_DIR)/usr/bin
	-install -m 0755 $(OUT_DIR)/rcwt_dump $(TARGET_DIR)/usr/bin

clean:
	find $(OUT_DIR) -name \*.o -exec rm -f {} \;

.PHONY: all install clean test install_test

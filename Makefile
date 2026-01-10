# 编译器设置
CXX := g++
DEBUG_FLAGS := -g -O0
RELEASE_FLAGS := -O2
CXXFLAGS := -std=c++20 -Iinclude -MMD $(DEBUG_FLAGS)

# 目录设置
SRC_DIR := src
INCLUDE_DIR := include
FRONTEND_DIR := $(SRC_DIR)/frontend
IR_DIR :=$(SRC_DIR)/ir
UTIL_DIR :=$(SRC_DIR)/util
VM_DIR :=$(SRC_DIR)/vm
OUTPUT_DIR := output

# 自动查找源文件和生成依赖
SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(FRONTEND_DIR)/*.cpp) \
		$(wildcard $(IR_DIR)/*.cpp) \
		$(wildcard $(UTIL_DIR)/*.cpp) \
# 		$(wildcard $(VM_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(SRC_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# 可执行文件路径
TARGET := $(OUTPUT_DIR)/main.exe

# 默认目标：先编译，再清理中间文件
all: $(TARGET) clean

# 创建输出目录
$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS) | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# 通用编译规则（自动处理依赖）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 包含自动生成的依赖文件
-include $(DEPS)

# 判断操作系统
ifeq ($(OS),Windows_NT)
    RM = del /F /Q
    RMDIR = rmdir /S /Q
    FIXPATH = $(subst /,\,$1)
else
    RM = rm -f
    RMDIR = rm -rf
    FIXPATH = $1
endif


# 清理中间文件（保留可执行文件）
clean:
	@echo "Cleaning intermediate files..."
	@$(RM) $(call FIXPATH,$(OBJS) $(DEPS)) 2>nul || exit 0

# 彻底清理（包括可执行文件）
distclean: clean
	@$(RM) $(call FIXPATH,$(TARGET)) 2>nul || exit 0
	@$(RMDIR) $(call FIXPATH,$(OUTPUT_DIR)) 2>nul || exit 0

.PHONY: all clean distclean

# 报错系统优化进度

## 已完成改动

### 1. include/error/error.h
- 新增 `ErrorCode` 枚举，包含唯一错误码：
  - Parser 语法错误 (1xxx): 18个错误码
  - Semantic 语义错误 (2xxx): 18个错误码
- `ComilerError` 类新增 `ErrorCode code` 字段
- `what()` 输出格式改为 `[E错误码] Error...`，方便定位

### 2. include/error/error_manager.h
- 新增统一接口 `syntaxError()` / `semanticError()`
- 保留所有旧 `emit_*` 方法作为**向后兼容**（内部添加了 ErrorCode）
- 总代码量从 191 行精简到 240 行（旧接口全部保留但加了功能）

### 3. include/frontend/parser.h
- 新增 4 个便捷报错宏：
  - `SYNTAX_ERR(code, msg)` — 统一语法错误 + return nullptr
  - `SYNTAX_ERR_LOC(code, msg, loc)` — 指定位置的错误
  - `MISS_TOKEN(token)` — 缺失 token 快捷宏
  - `UNEXPECTED_TOKEN()` — 意外 token 快捷宏

### 4. include/frontend/semantic.h
- 新增 `raiseError(ErrorCode, ...)` 重载方法
- 新增 3 个语义错误便捷宏：
  - `SEMANTIC_ERR(code, msg, loc)` — 统一语义错误 + return _errorType
  - `SEMANTIC_ERR_RET(code, msg, loc, ret)` — 语义错误 + return 指定值

## 编译结果
- ✔ 编译通过（0 warning, 0 error）
- ✔ test.ns 正确编译为 test.exe

## 待完成（可选，不影响现有功能）
- [ ] 逐步将 parser.cpp 中的旧 emit 调用改为新宏
- [ ] 逐步将 semantic.cpp 中的 raiseError + MARK_ERROR 模式改为 SEMANTIC_ERR 宏
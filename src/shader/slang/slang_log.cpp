#include "imports.h"
#include "slang_log.h"
#include "slang_utility.h"

#include <cstdio>
#include <cstdarg>



void
slang_info_log_construct(slang_info_log * log)
{
    log->text.clear();
    log->dont_free_text = false;
    log->error_flag = false;
}

void
slang_info_log_destruct(slang_info_log * log)
{
    log->text.clear();
}

static int
slang_info_log_message(slang_info_log * log, const char *prefix,
       const char *msg)
{
    if (log->dont_free_text)
return 0;
    if (prefix) {
log->text += prefix;
log->text += ": ";
    }
    log->text += msg;
    log->text += '\n';
    return 1;
}

int
slang_info_log_print(slang_info_log * log, const char *msg, ...)
{
    va_list va;
    char buf[1024];

    va_start(va, msg);
    vsnprintf(buf, sizeof(buf), msg, va);
    va_end(va);
    return slang_info_log_message(log, nullptr, buf);
}

int
slang_info_log_error(slang_info_log * log, const char *msg, ...)
{
    va_list va;
    char buf[1024];

    va_start(va, msg);
    vsnprintf(buf, sizeof(buf), msg, va);
    va_end(va);
    log->error_flag = true;
    if (slang_info_log_message(log, "Error", buf))
return 1;
    slang_info_log_memory(log);
    return 0;
}

int
slang_info_log_warning(slang_info_log * log, const char *msg, ...)
{
    va_list va;
    char buf[1024];

    va_start(va, msg);
    vsnprintf(buf, sizeof(buf), msg, va);
    va_end(va);
    if (slang_info_log_message(log, "Warning", buf))
return 1;
    slang_info_log_memory(log);
    return 0;
}

void
slang_info_log_memory(slang_info_log * log)
{
    if (!slang_info_log_message(log, "Error", "Out of memory.")) {
log->dont_free_text = true;
log->error_flag = true;
log->text = "Error: Out of memory.\n";
    }
}

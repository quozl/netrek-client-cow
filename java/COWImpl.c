/* Native method for calling COW from Java */

#include <cowapi.h>

#include "jri.h"
#define IMPLEMENT_COW
#include "COW.h"


JRI_PUBLIC_API(void)
native_COW_run(JRIEnv* env, struct COW* self, struct java_lang_String *server, jint port)
{
    char *s;
    char *name="COW Netrek plugin";

    s=JRI_GetStringUTFChars(env,server);
    if (s[0]='\0') s=0;       /* No name given: use defaults */

    cowmain(s, (int)port, name);

    return;
}


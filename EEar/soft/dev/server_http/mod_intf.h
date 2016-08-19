
#ifndef __MOD_INTF_H__
#define __MOD_INTF_H__


// the same in mod_intf.inc!!!
// these all functions are SINGLE-THREADED!

typedef struct {
const char* (WINAPI *GetCGIModName)();               // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_SERVER)(const char *name);     // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_GET)(const char *name);        // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_POST)(const char *name);       // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_COOKIE)(const char *name);     // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_REQUEST)(const char *name);    // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *_SESSION)(const char *name);    // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
void (WINAPI *Flush)(void);
void (WINAPI *Echo)(const char *text);
void (WINAPI *EchoBuff)(const void *buff,unsigned size);
void (WINAPI *Header)(const char *name,const char *value,BOOL b_replace);
void (WINAPI *SetRawCookie)(const char *name,const char *value,time_t expire_sec,const char *path,const char *domain);
BOOL (WINAPI *SessionStart)(unsigned wait_same_session_ms,const char *name,time_t server_expire_sec);
void (WINAPI *SessionReleaseOwnership)();
const char* (WINAPI *SessionGetName)();                               // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *SessionGetId)();                                 // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *SessionGetRootFromContentRelPathUnix)();         // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *SessionGetContentRelPathUnix)();                 // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *SessionGetContentFullPathWin)();                 // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
const char* (WINAPI *SessionGetVar)(const char *name);                // caller'у необходимо сразу скопировать возвращенную строку до вызова след. функции, кот. возвращает строку!
void (WINAPI *SessionSetVar)(const char *name,const char *value);
void (WINAPI *SessionEmptyVars)();
void (WINAPI *SessionEmptyContent)();
} TMODINTF;


#endif

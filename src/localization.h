#ifndef __N_LOCALIZATION_H__
#define __N_LOCALIZATION_H__

typedef struct Language_t
{
    const char* name;
    const char* json;
}Language_t;
void N_LocalizationLoadLanguages();
void N_LocalizationSetLanguage(int language);
char* N_LocalizationString(const char* name);
#endif
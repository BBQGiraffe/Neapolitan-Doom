#include <SFML/Window.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <unistd.h>
#include "neapolitan.h"
#include "m_keybinds.h"
#include "i_sound.h"

char bindnames[7][32] = {"Forward", "Backwards", "Left", "Right", "Use", "Fire", "Run"};
boolean useMouse = 0;
int keybindCount = 7;
//typing this out really makes me miss C#
char keynames[sfKeyCount][16] =
{
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z",
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "Escape", //unbindable
        "LControl",
        "LShift",
        "LAlt",
        "LSystem", //unbindable
        "RControl",
        "RShift",
        "RAlt",
        "RSystem",
        "Menu",
        "[",
        "]",
        ";",
        ",",
        ".",
        "\'",
        "/",
        "\\",
        "~",
        "=",
        "-",
        "Space",
        "Enter",
        "Backspace",
        "Tab",
        "Page Up",
        "Page Down",
        "End",
        "Home",
        "Insert",
        "Delete",
        "+",
        "-",
        "*",
        "/",
        "Left",
        "Right",
        "Up",
        "Down"

};

int unbindablekeys[] =
{
    sfKeyEscape,
    sfKeyLSystem,
    sfKeyRSystem,
    sfKeyReturn
};

int keybinds[] =
{
    KEY_UPARROW,
    KEY_DOWNARROW,
    KEY_LEFTARROW,
    KEY_RIGHTARROW,
    KEY_RCTRL,
    KEY_RSHIFT,
    KEY_SPACE
};

int unbindableKeyCount = 4;
int snd_DoPitchShift;

boolean fixInfiniteMonsterHeight = 1;

void N_WriteConfig()
{
    printf("writing config...\n");
    cJSON* json = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "key_up", key_up);
    cJSON_AddNumberToObject(json, "key_down", key_down);
    cJSON_AddNumberToObject(json, "key_left", key_left);
    cJSON_AddNumberToObject(json, "key_right", key_right);
    cJSON_AddNumberToObject(json, "key_fire", key_fire);
    cJSON_AddNumberToObject(json, "key_use", key_use);
    cJSON_AddNumberToObject(json, "key_run", key_speed);
    cJSON_AddBoolToObject(json, "mouse_look", useMouse);
    cJSON_AddBoolToObject(json, "monster_height_fix", fixInfiniteMonsterHeight);

    cJSON_AddStringToObject(json, "soundfont", soundfontName);
    cJSON_AddNumberToObject(json, "sfxVolume", snd_SfxVolume);
    cJSON_AddNumberToObject(json, "musVolume", snd_MusicVolume);


    char* text = cJSON_Print(json);

    FILE* file = fopen(NEAPOLITAN_SAVEFILE, "w");
    fwrite(text, strlen(text), 1, file);
    fclose(file);
}


void N_RebindKeys()
{
    key_up = keybinds[0];
    key_down = keybinds[1];
    key_left = keybinds[2];
    key_right = keybinds[3];
    key_fire = keybinds[4];
    key_speed = keybinds[5];
    key_use = keybinds[6];
}

#define DEFAULT_SOUNDFONT "chorium.sf2"

void N_LoadConfig(void)
{
    if(access(NEAPOLITAN_SAVEFILE, F_OK))
    {
        N_WriteConfig();
        return;
    }

    printf("N_LoadConfig: loading %s\n", NEAPOLITAN_SAVEFILE);
    char textbuffer[2048];
    FILE* file = fopen(NEAPOLITAN_SAVEFILE, "r");
    fread(&textbuffer, 2048, 1, file);
    fclose(file);
    cJSON* json = cJSON_Parse(textbuffer);


    //I love deserialization
    cJSON* j_key_up = cJSON_GetObjectItem(json, "key_up");
    cJSON* j_key_down = cJSON_GetObjectItem(json, "key_down");
    cJSON* j_key_left = cJSON_GetObjectItem(json, "key_right");
    cJSON* j_key_right = cJSON_GetObjectItem(json, "key_right");

    cJSON* j_key_fire = cJSON_GetObjectItem(json, "key_fire");
    cJSON* j_key_speed = cJSON_GetObjectItem(json, "key_speed");
    cJSON* j_key_use = cJSON_GetObjectItem(json, "key_use");


    cJSON* j_use_mouse = cJSON_GetObjectItem(json, "mouse_look");
    cJSON* j_fix_monster_height = cJSON_GetObjectItem(json, "monster_height_fix");
    cJSON* j_soundfont = cJSON_GetObjectItem(json, "soundfont");
    cJSON* j_sfx_volume = cJSON_GetObjectItem(json, "sfxVolume");
    cJSON* j_mus_volume = cJSON_GetObjectItem(json, "musVolume");

    //if json element for keybind is NULL, set it to default SFML key
    keybinds[0] = key_up = j_key_up ? j_key_up->valueint : KEY_UPARROW;
    keybinds[1] = key_down = j_key_down ? j_key_down->valueint : KEY_DOWNARROW;
    keybinds[2] = key_left = j_key_left ? j_key_left->valueint : KEY_LEFTARROW;
    keybinds[3] = key_right = j_key_right ? j_key_right->valueint : KEY_RIGHTARROW;
    keybinds[4] = key_fire = j_key_fire ? j_key_fire->valueint : KEY_RCTRL;
    keybinds[5] = key_speed = j_key_speed ? j_key_speed->valueint : KEY_RSHIFT;
    keybinds[6] = key_use = j_key_use ? j_key_use->valueint : KEY_SPACE;

    useMouse = j_use_mouse ? cJSON_IsTrue(j_use_mouse) : false;
    fixInfiniteMonsterHeight = j_fix_monster_height ? cJSON_IsTrue(j_fix_monster_height) : false;
    
    soundfontName = j_soundfont ? j_soundfont->valuestring : DEFAULT_SOUNDFONT;

    snd_SfxVolume = j_sfx_volume ? j_soundfont->valueint : 15;
    snd_MusicVolume = j_sfx_volume ? j_soundfont->valueint : 15;

    I_LoadSoundFont(soundfontName);
    I_SetMusicVolume(snd_MusicVolume);

    N_RebindKeys();
}


void N_MonsterHeightFix(int choice)
{
    fixInfiniteMonsterHeight = choice;
}
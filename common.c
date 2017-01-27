#include "common.h"
#include <minizip/unzip.h>
#include <file/file_path.h>
#include <unistd.h>
#include <string.h>
#include "mrboom.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "data.h"
#ifdef RETRO
#include "retro.h"
#include "libretro.h"
extern retro_log_printf_t log_cb;
#define log_error(...) log_cb(RETRO_LOG_ERROR,__VA_ARGS__);
#define log_debug(...) log_cb(RETRO_LOG_DEBUG,__VA_ARGS__);
#else
#define log_error(...) printf(__VA_ARGS__);
#define log_debug(...) printf(__VA_ARGS__);
#endif

#define NB_WAV 16
#define NB_VOICES 28
#define keyboardCodeOffset 32
#define keyboardReturnKey 28
#define keyboardExitKey 1
#define keyboardDataSize 8
#define nb_dyna 8

static Mix_Chunk * wave[NB_WAV];
static int ignoreForAbit[NB_WAV];
static int ignoreForAbitFlag[NB_WAV];


extern Memory m;

int rom_create(const char *path) {
    FILE * file = fopen(path, "wb");
    if (fwrite (dataRom , sizeof(char), sizeof(dataRom), file)!=sizeof(dataRom)) {
        log_error("create_rom error\n");
        return -1;
    }
    fclose(file);
    return 0;
}

int rom_unzip(const char *path, const char *extraction_directory)
{
    path_mkdir(extraction_directory);

    unzFile *zipfile = unzOpen(path);
    if ( zipfile == NULL )
    {
        log_error("%s: not found\n", path);
        return -1;
    }
    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
    {
        log_error("could not read file global info\n");
        unzClose(zipfile);
        return -1;
    }


    char read_buffer[8192];

    uLong i;
    for (i = 0; i < global_info.number_entry; ++i)
    {
        unz_file_info file_info;
        char filename[PATH_MAX_LENGTH];
        if (unzGetCurrentFileInfo(zipfile, &file_info, filename, PATH_MAX_LENGTH,
                                  NULL, 0, NULL, 0 ) != UNZ_OK)
        {
            log_error( "could not read file info\n" );
            unzClose( zipfile );
            return -1;
        }

        const size_t filename_length = strlen(filename);
        if (filename[filename_length-1] == '/')
        {
            log_debug("dir:%s\n", filename);
            char abs_path[PATH_MAX_LENGTH];
            fill_pathname_join(abs_path,
                               extraction_directory, filename, sizeof(abs_path));
            path_mkdir(abs_path);
        }
        else
        {
            log_debug("file:%s\n", filename);
            if (unzOpenCurrentFile(zipfile) != UNZ_OK)
            {
                log_error("could not open file\n");
                unzClose(zipfile);
                return -1;
            }

            char abs_path[PATH_MAX_LENGTH];
            fill_pathname_join(abs_path,
                               extraction_directory, filename, sizeof(abs_path));
            FILE *out = fopen(abs_path, "wb");
            if (out == NULL)
            {
                log_error("could not open destination file\n");
                unzCloseCurrentFile(zipfile);
                unzClose(zipfile);
                return -1;
            }

            int error = UNZ_OK;
            do
            {
                error = unzReadCurrentFile(zipfile, read_buffer, 8192);
                if (error < 0)
                {
                    log_error("error %d\n", error);
                    unzCloseCurrentFile(zipfile);
                    unzClose(zipfile);
                    return -1;
                }

                if (error > 0)
                    fwrite(read_buffer, error, 1, out);

            } while (error > 0);

            fclose(out);
        }

        unzCloseCurrentFile(zipfile);

        if (i + 1  < global_info.number_entry)
        {
            if (unzGoToNextFile(zipfile) != UNZ_OK)
            {
                log_error("cound not read next file\n");
                unzClose(zipfile);
                return -1;
            }
        }
    }
    unzClose(zipfile);
    unlink(path);
    return 0;
}

int mrboom_init(char * save_directory) {
    int i;
    char romPath[4096];
    char extractPath[4096];
    asm2C_init();
    m.taille_exe_gonfle=0;
    strcpy((char *) &m.iff_file_name,"mrboom31.dat");

    // Initialize SDL.
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        log_error("Error SDL_Init\n");
    }

    //Initialize SDL_mixer
    if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 512 ) == -1 ) {
        log_error("Error Mix_OpenAudio\n");
    }

    snprintf(romPath, sizeof(romPath), "%s/mrboom.rom", save_directory);
    snprintf(extractPath, sizeof(extractPath), "%s/mrboom", save_directory);
    log_debug("romPath: %s\n", romPath);

    rom_create(romPath);
    rom_unzip(romPath, extractPath);
    m.path=strdup(extractPath);

    for (i=0;i<NB_WAV;i++) {
        char tmp[PATH_MAX_LENGTH];
        sprintf(tmp,"%s/%d.WAV",extractPath,i);
        wave[i] = Mix_LoadWAV(tmp);
        unlink(tmp);
        ignoreForAbit[i]=0;
        ignoreForAbitFlag[i]=0;
        if (wave[i]==NULL) {
            log_error( "cant load %s\n",tmp);
        }
    }
    ignoreForAbitFlag[0]=30;
    ignoreForAbitFlag[10]=30; // kanguru jump
    ignoreForAbitFlag[13]=30;
    ignoreForAbitFlag[14]=30;

    for (i=0;i<keyboardDataSize*nb_dyna;i++) {
        if (!((i+1)%keyboardDataSize)) {
            m.touches_[i]=-1;
        } else {
            m.touches_[i]=i+keyboardCodeOffset;
        }
    }
    return 0;
}

void mrboom_deinit() {
    int i;
    /* free WAV */
    for (i=0;i<NB_WAV;i++) {
        Mix_FreeChunk(wave[i]);
    }
    // quit SDL_mixer
    Mix_CloseAudio();
}

void mrboom_play_fx() {
    int i;
    static int last_voice=0;
    for (i=0;i<NB_WAV;i++) {
        if (ignoreForAbit[i]) {
            ignoreForAbit[i]--;
        }
    }
    while (m.last_voice!=last_voice) {
        db a=READDBlW(blow_what2[last_voice/2]);
        db a2=a>>4;
        db a1=a&0xf;
        db b=READDBhW(blow_what2[last_voice/2]);
        log_debug("blow what: sample = %d / panning %d, note: %d ignoreForAbit[%d]\n",a1,a2,b,ignoreForAbit[a1]);
        last_voice=(last_voice+2)%NB_VOICES;
        if ((a1>=0) && (a1<NB_WAV) && (wave[a1]!=NULL)) {
            bool dontPlay=0;
            if (ignoreForAbit[a1]) {
                log_debug("Ignore sample id %d\n",a1);
                dontPlay=1;
            }
            if (dontPlay == 0) {
                if ( Mix_PlayChannel(-1, wave[a1], 0) == -1 ) {
                    log_error("Error playing sample id %d.\n",a1);
                }
                // special message on failing to start a game...
                if (a1==14) {
#ifdef RETRO
                    show_message("2 players are needed to start!");
#endif
                }
                ignoreForAbit[a1]=ignoreForAbitFlag[a1];
            }
        } else {
            log_error("Wrong sample id %d or NULL.",a1);
        }
    }
}


void mrboom_update_input(int keyid, int port,int state) {
    int key=-1;
    int keyAdder=keyboardCodeOffset+port*keyboardDataSize;
#define keyboardExtraSelectStartKeysSize 2
#define offsetExtraKeys keyboardDataSize*nb_dyna+keyboardCodeOffset
    switch (keyid) {
        case button_down:
            key=3+keyAdder; // DOWN
            break;
        case button_right:
            key=1+keyAdder; //right
            break;
        case button_left:
            key=0+keyAdder; //left
            break;
        case button_up:
            key=2+keyAdder; //up
            break;
        case button_a:
            key=5+keyAdder; //bouton 2
            break;
        case button_select:
            key=offsetExtraKeys+port*keyboardExtraSelectStartKeysSize; // selection;
            break;
        case button_start:
            key=offsetExtraKeys+port*keyboardExtraSelectStartKeysSize+1; // start;
            break;
        case button_b:
            key=4+keyAdder; //bouton 1
            break;
        case button_y:
            key=6+keyAdder; //bouton 3
            break;
        case button_x:
            key=6+keyAdder; //bouton 3
            break;
        case button_l:
            key=4+keyAdder; //bouton 1
            break;
        case button_r:
            key=4+keyAdder; //bouton 1
            break;
    }
    if (key!=-1) {
        int i;
        log_debug("pressing %d\n",key);
        m.clavier[key]=state;
        m.une_touche_a_telle_ete_pressee=1;
        m.clavier[keyboardReturnKey]=0; // return
        m.clavier[keyboardExitKey]=0; //esc
        for (i=0;i<nb_dyna;i++) {
            if (m.clavier[offsetExtraKeys+i*2] && m.clavier[offsetExtraKeys+1+i*2]) {
                // select + start -> escape
                m.clavier[keyboardExitKey]=1;
                log_debug("exit key pressed...\n");
                m.sortie=1;

            }
            if (m.clavier[offsetExtraKeys+i*2+1]) {
                log_debug("return key pressed...\n");
                m.clavier[keyboardReturnKey]=1; // return
            }
        }
    } else {
        log_debug("unknown %d\n,key",key);
    }
}

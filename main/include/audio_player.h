#include "acodecs.h"
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_FILENAME 40
#define MAX_SONGS 1024

typedef struct Entry {
	char *name;   /** File name */
	off_t size;   /** File size in bytes */
	mode_t mode;  /** Filetype and permissions */
	time_t mtime; /** Modifictaion time. */
} Entry;

typedef enum FileType {
	FileTypeNone,
	FileTypeFolder,
	FileTypeMP3,
	FileTypeOGG,
	FileTypeMOD,
	FileTypeWAV,
	FileTypeFLAC,
	FileTypeGME,

	FileTypeJPEG,
	FileTypePNG,
	FileTypeGIF,
	FileTypeBMP,

	FileTypeNES,
	FileTypeGB,
	FileTypeGBC,
	FileTypeSMS,
	FileTypeCOL,
	FileTypeGG,
} FileType;

typedef struct AudioPlayerParam {
	Entry *entries;
	int n_entries;
	int index;
	const char *cwd;
	bool play_all;
} AudioPlayerParam;

typedef struct {
  char *filename;
  char *filepath;
  AudioCodec codec;
} Song;

// Cmds sent to player task for control
typedef enum PlayerCmd {
  PlayerCmdNone,
  PlayerCmdTerminate,
  PlayerCmdPause,
  PlayerCmdNext,
  PlayerCmdPrev,
  PlayerCmdReinitAudio,
  PlayerCmdToggleLoopMode,
} PlayerCmd;

typedef enum PlayingMode {
  PlayingModeNormal = 0,
  PlayingModeRepeatSong,
  PlayingModeRepeatPlaylist,
  PlayingModeMax,
} PlayingMode;

typedef struct PlayerState
{
	bool playing;

	Song *playlist;
	size_t playlist_length;

	int playlist_index;

	PlayingMode playing_mode;
} PlayerState;

typedef enum PlayerResult
{
	PlayerResultNone = 0,
	PlayerResultError,

	PlayerResultDone,
	PlayerResultNextSong,
	PlayerResultPrevSong,
	PlayerResultStop,
} PlayerResult;

/**
 * @brief Send a command to the audio player task.
 *
 * Sends a command to the player task via queue for controlling playback.
 *
 * @param cmd The command to send.
 */
void player_send_cmd(PlayerCmd cmd);

/**
 * @brief Start the audio player task.
 *
 * Initializes queues and creates the player task to begin audio playback.
 */
void player_start(void);

/**
 * @brief Terminate the audio player task.
 *
 * Sends a terminate command and waits for the task to finish.
 */
void player_terminate(void);

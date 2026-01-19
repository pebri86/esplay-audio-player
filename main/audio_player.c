#include "audio_player.h"
#include "acodecs.h"
#include "audio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"
#include "mp3_metadata.h"
#include "ui_player.h"

static const char *TAG = "audio player";
extern app_context_t app_ctx;
PlayerState player_state = {
    0,
};
static AudioCodec choose_codec(FileType ftype);
static PlayerCmd player_poll_cmd(void);
static void player_cmd_ack(void);
static void player_teardown_task(void);
static void free_playlist(PlayerState *state);
static FileType fops_determine_filetype(Entry *entry);
static int make_playlist(PlayerState *state, const AudioPlayerParam params);
static Song *create_song_from_entry(Entry *entry, const char *cwd);
#define DECODER_ERROR(acodec, ...) do { \
  decoder->close(acodec); \
  ESP_LOGE(TAG, __VA_ARGS__); \
  return PlayerResultError; \
} while(0)
static int get_next_song_index(PlayerState *state, PlayerResult res, int current_index);
static int fops_list_dir(Entry **entriesp, const char *cwd);
static int entry_cmp(const void *a, const void *b);
static void player_task(void *arg);

static bool player_task_running = false;

static QueueHandle_t player_cmd_queue;
static QueueHandle_t player_ack_queue;
static TaskHandle_t audio_player_task_handle;

static int entry_cmp(const void *a, const void *b)
{
  int isdir1, isdir2, cmpdir;
  const Entry *r1 = a;
  const Entry *r2 = b;
  isdir1 = S_ISDIR(r1->mode);
  isdir2 = S_ISDIR(r2->mode);
  cmpdir = isdir2 - isdir1;
  return cmpdir ? cmpdir : strcoll(r1->name, r2->name);
}

/**
 * @brief Choose the appropriate audio codec based on file type.
 *
 * Maps file type to audio codec for decoding.
 *
 * @param ftype The file type.
 * @return The corresponding audio codec.
 */
static AudioCodec choose_codec(FileType ftype)
{
  switch (ftype)
  {
  case FileTypeMOD:
    return AudioCodecMOD;
  case FileTypeMP3:
    return AudioCodecMP3;
  case FileTypeOGG:
    return AudioCodecOGG;
  case FileTypeFLAC:
    return AudioCodecFLAC;
  case FileTypeWAV:
    return AudioCodecWAV;
  case FileTypeGME:
    return AudioCodecGME;
  default:
    return AudioCodecUnknown;
  }
}

/**
 * @brief Check if the file extension matches any in the list.
 *
 * @param filename The filename.
 * @param len Length of filename.
 * @param exts Array of extensions (null-terminated).
 * @param ext_lens Array of extension lengths.
 * @return true if matches, false otherwise.
 */
static bool matches_extension(const char *filename, size_t len, const char **exts, const int *ext_lens)
{
  for (int i = 0; exts[i] != NULL; i++)
  {
    int ext_len = ext_lens[i];
    if (len >= (size_t)ext_len && !strncasecmp(exts[i], &filename[len - ext_len], ext_len))
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief Determine the file type based on file extension.
 *
 * Checks the file extension to classify the file type.
 *
 * @param entry The file entry.
 * @return The determined file type.
 */
static FileType fops_determine_filetype(Entry *entry)
{
  const char *filename = entry->name;
  size_t len = strlen(filename);
  if (len < 4)
  {
    return FileTypeNone;
  }

  // Common music formats
  if (matches_extension(filename, len, (const char *[]){"mp3", NULL}, (const int[]){3}))
  {
    return FileTypeMP3;
  }
  else if (matches_extension(filename, len, (const char *[]){"ogg", NULL}, (const int[]){3}))
  {
    return FileTypeOGG;
  }
  else if (matches_extension(filename, len, (const char *[]){"xm", "mod", "s3m", "it", NULL}, (const int[]){2, 3, 3, 2}))
  {
    return FileTypeMOD;
  }
  else if (matches_extension(filename, len, (const char *[]){"wav", NULL}, (const int[]){3}))
  {
    return FileTypeWAV;
  }
  else if (matches_extension(filename, len, (const char *[]){"flac", NULL}, (const int[]){4}))
  {
    return FileTypeFLAC;
  }
  return FileTypeNone;
}

/**
 * @brief Poll for a command from the player command queue.
 *
 * Receives a command from the queue without blocking.
 *
 * @return The received command or PlayerCmdNone if none.
 */
static PlayerCmd player_poll_cmd(void)
{
  PlayerCmd polled_cmd = PlayerCmdNone;

  xQueueReceive(player_cmd_queue, &polled_cmd, 0);
  return polled_cmd;
}

/**
 * @brief Acknowledge a command by sending to the ack queue.
 *
 * Sends an acknowledgment to indicate command processing is complete.
 */
static void player_cmd_ack(void)
{
  int tmp = 42;
  xQueueSend(player_ack_queue, &tmp, 0);
}

/**
 * @brief Teardown the player task.
 *
 * Deletes the task watchdog and terminates the task.
 */
static void player_teardown_task(void)
{
  esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
  vTaskDelete(NULL);
  free_playlist(&player_state);
}

/**
 * @brief Handle a received player command.
 *
 * Processes the command and updates the player state accordingly.
 *
 * @param state The current player state.
 * @param info The current audio info.
 * @param received_cmd The command to handle.
 * @return The result of handling the command.
 */
static PlayerResult handle_cmd(PlayerState *const state, const AudioInfo info,
                                const PlayerCmd received_cmd)
{
  if (received_cmd == PlayerCmdNone)
  {
    return PlayerResultDone;
  }
  ESP_LOGI(TAG, "Received cmd: %d", received_cmd);
  PlayerResult res = PlayerResultDone;
  switch (received_cmd)
  {
  case PlayerCmdPause:
    state->playing = !state->playing;
    if (state->playing)
    {
      audio_init((int)info.sample_rate);
    }
    else
    {
      audio_terminate();
    }
    break;
  case PlayerCmdReinitAudio:
    if (state->playing)
      audio_terminate();
    audio_init((int)info.sample_rate);
    break;
  case PlayerCmdToggleLoopMode:
    state->playing_mode = (state->playing_mode + 1) % PlayingModeMax;
    break;
  case PlayerCmdTerminate:
    res = PlayerResultStop;
    break;
  case PlayerCmdNext:
    res = PlayerResultNextSong;
    break;
  case PlayerCmdPrev:
    res = PlayerResultPrevSong;
    break;
  case PlayerCmdNone:
    break;
  }
  player_cmd_ack();
  return res;
}

/**
 * @brief Set the metadata for the current song in the UI.
 *
 * Extracts and displays metadata for MP3 files or uses filename/artist fallback.
 *
 * @param song The current song.
 */
static void set_metadata(const Song *const song)
{
  if (song->codec == AudioCodecMP3)
  {
    mp3_metadata_t meta;
    mp3_read_metadata(song->filepath, &meta);
    ui_player_set_metadata(meta.title, meta.artist, &app_ctx);
  }
  else
    ui_player_set_metadata(song->filename, "Unknown Artist", &app_ctx);
}

/**
 * @brief Play a single song.
 *
 * Opens the song file, decodes and plays it, handling commands during playback.
 *
 * @param song The song to play.
 * @param audio_buf The audio buffer to use.
 * @return The result of playback.
 */
static PlayerResult play_song(const Song *const song, int16_t *audio_buf)
{
  PlayerState *state = &player_state;
  AudioInfo info;
  void *acodec = NULL;
  set_metadata(song);
  ESP_LOGI(TAG, "Playing file: %s, codec: %d\n", song->filepath, song->codec);
  AudioDecoder *decoder = acodec_get_decoder(song->codec);
  if (decoder == NULL)
  {
    ESP_LOGE(TAG, "error determining deocer for song %s\n", song->filepath);
    return PlayerResultError;
  }

  if (decoder->open(&acodec, song->filepath) != 0)
  {
    ESP_LOGE(TAG, "error opening song %s\n", song->filepath);
    return PlayerResultError;
  }
  if (decoder->get_info(acodec, &info) != 0)
  {
    DECODER_ERROR(acodec, "error retreiving song info %s\n", song->filepath);
  }

  // Assume audio_buf is pre-allocated and large enough
  audio_init((int)info.sample_rate);

  int n_frames = 0;
  state->playing = true;
  PlayerResult result = PlayerResultDone;
  ESP_LOGI(TAG, "starting to play audio...\n");
  do
  {
    esp_task_wdt_reset();
    if ((result = handle_cmd(state, info, player_poll_cmd())) !=
        PlayerResultDone)
    {
      break;
    }

    if (state->playing)
    {
      n_frames =
          decoder->decode(acodec, audio_buf, (int)info.channels, info.buf_size);
      audio_submit(audio_buf, n_frames);
    }
    else
    {
      usleep(10 * 1000);
    }
  } while (n_frames > 0);

  decoder->close(acodec);

  if (state->playing)
  {
    audio_terminate();
  }
  return result;
}

/**
 * @brief List directory entries.
 *
 * Reads the directory and populates an array of Entry structures.
 *
 * @param entriesp Pointer to store the array of entries.
 * @param cwd The current working directory path.
 * @return The number of entries or -1 on error.
 */
static int fops_list_dir(Entry **entriesp, const char *cwd)
{
  DIR *dp;
  struct dirent *ep;
  Entry *entries = NULL;
  int capacity = 0;
  int n = 0;

  if (!(dp = opendir(cwd)))
  {
    *entriesp = NULL;
    return -1;
  }

  while ((ep = readdir(dp)))
  {
    if (!strncmp(ep->d_name, ".", 2) || !strncmp(ep->d_name, "..", 3))
      continue;

    if (n >= capacity)
    {
      capacity = capacity == 0 ? 16 : capacity * 2;
      entries = realloc(entries, capacity * sizeof(*entries));
      if (!entries)
      {
        closedir(dp);
        return -1;
      }
    }

    const size_t fname_size = strlen(ep->d_name) + 1;
    entries[n].name = malloc(fname_size);
    if (!entries[n].name)
    {
      // Cleanup on failure
      for (int j = 0; j < n; j++)
        free(entries[j].name);
      free(entries);
      closedir(dp);
      return -1;
    }
    strncpy(entries[n].name, ep->d_name, fname_size);
    entries[n].size = 0;
    entries[n].mtime = 0;
    entries[n].mode = 0;
    if (ep->d_type == DT_REG)
    {
      entries[n].mode = S_IFREG;
    }
    else if (ep->d_type == DT_DIR)
    {
      entries[n].mode = S_IFDIR;
    }

    n++;
  }

  closedir(dp);
  qsort(entries, n, sizeof(*entries), entry_cmp);
  *entriesp = entries;
  return n;
}

/**
 * @brief Get the next song index based on the result.
 *
 * @param state The player state.
 * @param res The result from playing the song.
 * @param current_index The current song index.
 * @return The next song index.
 */
static int get_next_song_index(PlayerState *state, PlayerResult res, int current_index)
{
  if (res == PlayerResultDone || res == PlayerResultNextSong)
  {
    if (state->playing_mode != PlayingModeRepeatSong || res == PlayerResultNextSong)
    {
      current_index = (current_index + 1) % (int)state->playlist_length;
    }
  }
  else if (res == PlayerResultPrevSong)
  {
    if (--current_index < 0)
    {
      current_index = (int)state->playlist_length - 1;
    }
  }
  else if (res == PlayerResultError)
  {
    current_index = (current_index + 1) % (int)state->playlist_length;
  }
  return current_index;
}

/**
 * @brief The main audio player task.
 *
 * Runs the playback loop, handling song transitions and commands.
 *
 * @param arg Unused argument.
 */
static void player_task(void *arg)
{
  player_task_running = true;
  player_state.playing = false;
  memset(&player_state, 0, sizeof(PlayerState));
  Entry *new_entries;
  int n_entries = fops_list_dir(&new_entries, AUDIO_FILE_PATH);
  if (n_entries < 0)
  {
    ESP_LOGE(TAG, "Failed to list audio directory");
    return;
  }
  AudioPlayerParam params = {new_entries, n_entries, 0, AUDIO_FILE_PATH,
                             true};

  if (make_playlist(&player_state, params) != 0)
  {
    ESP_LOGI(TAG, "Could not determine audio codec\n");
    return;
  }
  struct PlayerState *state = &player_state;
  ESP_LOGI(TAG, "Playing playlist of length: %zu\n", state->playlist_length);

  // Allocate audio buffer once for reuse
  const size_t max_buf_size = 16384; // Adjust based on needs
  int16_t *audio_buf = calloc(1, max_buf_size * sizeof(int16_t));
  if (!audio_buf)
  {
    ESP_LOGE(TAG, "Failed to allocate audio buffer");
    return;
  }

  for (;;)
  {
    int song_index = state->playlist_index;
    Song *const song = &state->playlist[song_index];
    PlayerResult res = play_song(song, audio_buf);

    if (res == PlayerResultStop)
    {
      break;
    }
    state->playlist_index = get_next_song_index(state, res, song_index);
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  free(audio_buf);
  player_task_running = false;
  player_teardown_task();
}

/**
 * @brief Create a Song structure from an Entry.
 *
 * Allocates and initializes a Song from the given entry and cwd.
 *
 * @param entry The file entry.
 * @param cwd The current working directory.
 * @return Pointer to the created Song, or NULL on failure.
 */
static Song *create_song_from_entry(Entry *entry, const char *cwd)
{
  Song *song = malloc(sizeof(Song));
  if (!song)
    return NULL;

  AudioCodec codec = choose_codec(fops_determine_filetype(entry));
  song->codec = codec;

  char pathbuf[PATH_MAX];
  int printed = snprintf(pathbuf, PATH_MAX, "%s/%s", cwd, entry->name);
  song->filename = malloc(strlen(entry->name) + 1);
  if (!song->filename)
  {
    free(song);
    return NULL;
  }
  song->filepath = malloc((size_t)(printed + 1));
  if (!song->filepath)
  {
    free(song->filename);
    free(song);
    return NULL;
  }
  strncpy(song->filename, entry->name, strlen(entry->name) + 1);
  strncpy(song->filepath, pathbuf, (size_t)(printed + 1));
  return song;
}

/**
 * @brief Create a playlist from directory entries.
 *
 * Builds a playlist of songs based on the provided parameters.
 *
 * @param state The player state to update.
 * @param params The parameters for playlist creation.
 * @return 0 on success, -1 on failure.
 */
static int make_playlist(PlayerState *state, const AudioPlayerParam params)
{

  if (params.play_all)
  {
    size_t start_song = 0;
    size_t n_songs = 0;
    size_t song_indices[MAX_SONGS];
    for (size_t i = 0, j = 0; i < (size_t)params.n_entries; i++)
    {
      Entry *entry = &params.entries[i];
      AudioCodec codec = choose_codec(fops_determine_filetype(entry));
      if (codec != AudioCodecUnknown)
      {
        if ((size_t)params.index == i)
        {
          start_song = j;
        }
        song_indices[j++] = i;
        n_songs++;
      }
    }

    state->playlist = malloc(n_songs * sizeof(Song));
    if (!state->playlist)
      return -1;
    state->playlist_length = n_songs;

    for (size_t i = 0; i < n_songs; i++)
    {
      Entry *entry = &params.entries[song_indices[i]];
      Song *song = create_song_from_entry(entry, params.cwd);
      if (!song)
        return -1;
      state->playlist[i] = *song;
      free(song); // since we copied
    }
    state->playlist_index = (int)start_song;
  }
  else
  {
    Entry *entry = &params.entries[params.index];
    AudioCodec codec = choose_codec(fops_determine_filetype(entry));
    if (codec == AudioCodecUnknown)
    {
      return -1;
    }

    state->playlist = malloc(1 * sizeof(Song));
    if (!state->playlist)
      return -1;
    Song *song = create_song_from_entry(entry, params.cwd);
    if (!song)
    {
      free(state->playlist);
      return -1;
    }
    state->playlist[0] = *song;
    free(song);
    state->playlist_length = 1;
  }

  return 0;
}

/**
 * @brief Free the memory allocated for the playlist.
 *
 * Deallocates the song structures and their strings.
 *
 * @param state The player state containing the playlist.
 */
static void free_playlist(PlayerState *state)
{
  const size_t len = state->playlist_length;
  if (len < 1)
    return;
  for (size_t i = 0; i < len; i++)
  {
    free(state->playlist[i].filepath);
    free(state->playlist[i].filename);
  }
  free(state->playlist);
}

void player_send_cmd(PlayerCmd cmd)
{
  xQueueSend(player_cmd_queue, &cmd, 0);
  int tmp;
  xQueueReceive(player_ack_queue, &tmp, 10 / portTICK_PERIOD_MS);
}

void player_start()
{
  player_cmd_queue = xQueueCreate(3, sizeof(PlayerCmd));
  player_ack_queue = xQueueCreate(3, sizeof(int));
  const int stacksize = 9 * 8192;
  if (xTaskCreatePinnedToCore(
          player_task,               // The function the task will run
          "player_task",             // A descriptive name
          stacksize,                 // Stack depth (in words)
          NULL,                      // Parameters to pass to the function
          AUDIO_PLAYER_PRIORITY,     // Task priority
          &audio_player_task_handle, // Handle to reference the task
          AUDIO_PLAYER_CORE_ID       // The core to pin (0 or 1)
          ) != pdPASS)
  {
    ESP_LOGE(TAG, "Error creating task\n");
    return;
  }
  esp_task_wdt_add(audio_player_task_handle);
}

void player_terminate(void)
{
  if (!player_task_running)
  {
    return;
  }
  ESP_LOGI(TAG, "Trying to terminate player..\n");
  PlayerCmd term = PlayerCmdTerminate;
  xQueueSend(player_cmd_queue, &term, portMAX_DELAY);
  while (player_task_running)
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  ESP_LOGI(TAG, "Terminated?\n");
}

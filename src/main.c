#include <reminder.h>

struct data {
  struct pw_main_loop *loop;
  struct pw_stream *stream;
  double accumulator;
  uint64_t curr_frames;
  uint64_t max_frames;
};

/* Pipewire boilerplate code taken from:
   https://docs.pipewire.org/page_tutorial4.html */
static void on_process(void *userdata) {
  struct data *data = userdata;
  struct pw_buffer *b;
  struct spa_buffer *buf;
  int i, c, n_frames, stride;
  int16_t *dst, val;

  if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  buf = b->buffer;
  if ((dst = buf->datas[0].data) == NULL)
    return;

  stride = sizeof(int16_t) * DEFAULT_CHANNELS;
  n_frames = buf->datas[0].maxsize / stride;
  if (b->requested)
    n_frames = SPA_MIN(b->requested, n_frames);

  for (i = 0; i < n_frames; i++) {
    if (data->curr_frames >= data->max_frames) {
      val = 0;
    } else {
      data->accumulator += M_PI_M2 * 440 / DEFAULT_RATE;
      if (data->accumulator >= M_PI_M2)
        data->accumulator -= M_PI_M2;

      val = sin(data->accumulator) * DEFAULT_VOLUME * 32767.0;
      data->curr_frames++;
    }
    for (c = 0; c < DEFAULT_CHANNELS; c++)
      *dst++ = val;
  }

  buf->datas[0].chunk->offset = 0;
  buf->datas[0].chunk->stride = stride;
  buf->datas[0].chunk->size = n_frames * stride;

  pw_stream_queue_buffer(data->stream, b);
  if (data->curr_frames >= data->max_frames)
    pw_main_loop_quit(data->loop);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

void detach() {
  pid_t pid = fork();
  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS);
  if (setsid() < 0)
    exit(EXIT_FAILURE);
  chdir("/");
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("Usage:      reminder time message silent\n");
    printf("Example:    reminder 12:00 \"Buy me a coffee!: "
           "https://ko-fi.com/helenana\" true\n\n");
    printf("Arguments:\n");
    printf("time        Required. must be formatted as HH:MM.\n");
    printf("message     Dictates notification message, in case of "
           "multi-word must be encased in double quotes.\n");
    printf("silent      Disables notification sound.\n");
    return 1;
  }
  if (strlen(argv[1]) != 5) {
    printf("Error: time not properly formatted.\n");
    printf("Must be formatted as HH:MM");
    return 1;
  }
  char hinput[3];
  char minput[3];
  hinput[0] = argv[1][0];
  hinput[1] = argv[1][1];
  hinput[2] = '\0';
  minput[0] = argv[1][3];
  minput[1] = argv[1][4];
  minput[2] = '\0';
  int h2 = atoi(hinput);
  int m2 = atoi(minput);

  time_t t = time(NULL);
  struct tm *lt = localtime(&t);
  int h1 = lt->tm_hour;
  int m1 = lt->tm_min;

  // midnight crossover
  if (h2 < h1 || ((h2 == h1) && (m2 < m1))) {
    m2 += 1440;
  }

  int tm = (h2 - h1) * 60 + (m2 - m1);
  // printf("Sleeping for %d minutes, or %d seconds", tm, tm * 60);
  detach();
  sleep(tm * 60);

  notify_init("reminder");
  NotifyNotification *notif =
      notify_notification_new("Reminder!", argv[2], "dialog-information");
  notify_notification_show(notif, NULL);
  g_object_unref(G_OBJECT(notif));
  notify_uninit();

  if (argc == 4)
    // if silent...
    return 0;
  struct data data = {
      0,
  };
  data.max_frames = DEFAULT_RATE; // 1 sec?
  const struct spa_pod *params[1];
  uint8_t buffer[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

  pw_init(&argc, &argv);

  data.loop = pw_main_loop_new(NULL);

  data.stream = pw_stream_new_simple(
      pw_main_loop_get_loop(data.loop), "audio-src",
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                        "Playback", PW_KEY_MEDIA_ROLE, "Music", NULL),
      &stream_events, &data);

  params[0] = spa_format_audio_raw_build(
      &b, SPA_PARAM_EnumFormat,
      &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_S16,
                               .channels = DEFAULT_CHANNELS,
                               .rate = DEFAULT_RATE));

  pw_stream_connect(data.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
                    PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS |
                        PW_STREAM_FLAG_RT_PROCESS,
                    params, 1);

  pw_main_loop_run(data.loop);

  pw_stream_destroy(data.stream);
  pw_main_loop_destroy(data.loop);

  return 0;
}

#include <reminder.h>

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
    printf("Usage: reminder time message\n");
    printf("Time must be formatted as HH:MM\n");
    printf("Message is optional and in case of multi-word must be encased in double quotes.");
    return 1;
  }
  if (strlen(argv[1]) != 5) {
    printf("Error: time not properly formatted.");
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
  printf("Sleeping for %d minutes, or %d seconds", tm, tm*60);
  detach();
  sleep(tm*60);

  notify_init("reminder");
  NotifyNotification *notif = notify_notification_new(
      "Reminder!", argv[2], "dialog-information");
  notify_notification_show(notif, NULL);
  g_object_unref(G_OBJECT(notif));
  notify_uninit();
  return 0;
}

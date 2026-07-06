#if !defined(NCM_READLINE_H)
#define NCM_READLINE_H

char *ncm_readline_prompt(char *prompt);
void ncm_readline_remember(char *line);
void ncm_readline_release(char *line);

#endif /* NCM_READLINE_H */

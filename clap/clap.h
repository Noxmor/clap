#ifndef CLAP_H
#define CLAP_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define CLAP_FALSE 0
#define CLAP_TRUE 1

#ifndef CLAP_FLAG_CAPACITY
#define CLAP_FLAG_CAPACITY 256
#endif

#ifndef CLAP_SUBCOMMAND_CAPACITY
#define CLAP_SUBCOMMAND_CAPACITY 256
#endif

#define CLAP_FLAG_NO_LONG NULL
#define CLAP_FLAG_NO_SHORT 0

typedef enum
{
	CLAP_FLAG_NO_ARG = 0,
	CLAP_FLAG_OPT_ARG,
	CLAP_FLAG_REQ_ARG
} CLAPflagArgType;

typedef struct
{
	const char* long_name;
	char short_name;
	CLAPflagArgType arg_type;
	const char* subcommand_name;

} CLAPflag;

typedef struct
{
	const char* name;
} CLAPsubcommand;

typedef struct
{
	CLAPflag flags[CLAP_FLAG_CAPACITY];
	size_t flag_count;

	CLAPsubcommand subcommands[CLAP_SUBCOMMAND_CAPACITY];
	size_t subcommand_count;

	const char* current_arg;
	size_t current_arg_len;
	size_t arg_index;
	size_t current_arg_index;

	CLAPflag* current_flag;
	const char* current_flag_arg;
	CLAPsubcommand* subcommand;
	const char* current_non_opt;
	uint8_t next_is_no_opt;
} CLAPhandler;

static CLAPhandler handler;

void clapRegisterFlag(const char* long_name, char short_name, CLAPflagArgType arg_type, const char* subcommand_name)
{
	if (handler.flag_count >= CLAP_FLAG_CAPACITY)
	{
		printf("Could not register flag: Flag capacity exceeded!\n");
		return;
	}

	if (long_name == CLAP_FLAG_NO_LONG && short_name == CLAP_FLAG_NO_SHORT)
	{
		printf("Could not register flag: Invalid long name (CLAP_FLAG_NO_LONG) and short name (CLAP_FLAG_NO_SHORT)!\n");
		return;
	}

	if (long_name != CLAP_FLAG_NO_LONG)
	{
		for (size_t i = 0; i < strlen(long_name); i++)
		{
			if (!isalnum(long_name[i]) && long_name[i] != '-')
			{
				printf("Could not register flag: Invalid long name (%s)!\n", long_name);
				return;
			}
		}
	}

	if (short_name != CLAP_FLAG_NO_SHORT && !isalpha(short_name))
	{
		printf("Could not register flag: Invalid short name (%c)!\n", short_name);
		return;
	}

	if (arg_type < 0 || arg_type > CLAP_FLAG_REQ_ARG)
	{
		printf("Could not register flag: Invalid arg type!\n");
		return;
	}

	CLAPflag* flag = &handler.flags[handler.flag_count++];
	flag->long_name = long_name;
	flag->short_name = short_name;
	flag->arg_type = arg_type;
	flag->subcommand_name = subcommand_name;
}

void clapRegisterSubcommand(const char* name)
{
	if (handler.subcommand_count >= CLAP_SUBCOMMAND_CAPACITY)
	{
		printf("Could not register subcommand: Subcommand capacity exceeded!\n");
		return;
	}

	CLAPsubcommand* subcommand = &handler.subcommands[handler.subcommand_count++];
	subcommand->name = name;
}

uint8_t clapParsedFlag(const char* long_name, char short_name)
{
	if (!handler.current_flag || handler.subcommand)
		return CLAP_FALSE;

	if (handler.current_flag->long_name == CLAP_FLAG_NO_LONG || long_name == CLAP_FLAG_NO_LONG)
		return handler.current_flag->long_name == long_name && handler.current_flag->short_name == short_name;

	return strcmp(handler.current_flag->long_name, long_name) == 0 && handler.current_flag->short_name == short_name;
}

uint8_t clapParsedSubcommandFlag(const char* subcommand_name, const char* long_name, char short_name)
{
	if (!handler.current_flag || !handler.subcommand)
		return CLAP_FALSE;

	if (strcmp(handler.subcommand->name, subcommand_name) != 0)
		return CLAP_FALSE;

	return strcmp(handler.current_flag->long_name, long_name) == 0 && handler.current_flag->short_name == short_name;
}

uint8_t clapParsedSubcommand(const char* name)
{
	if(!handler.subcommand || !name)
		return CLAP_FALSE;

	return strcmp(handler.subcommand->name, name) == 0;
}

uint8_t clapParsedNonOpt(const char** arg)
{
	if (!handler.current_non_opt)
		return CLAP_FALSE;

	*arg = handler.current_non_opt;
	return CLAP_TRUE;
}

const char* clapGetArg(void)
{
	if (!handler.current_flag)
	{
		printf("Tried to access flag arg, but flag is NULL!\n");
		return NULL;
	}

	if (handler.current_flag->arg_type == CLAP_FLAG_NO_ARG)
	{
		if (handler.current_flag->long_name)
			printf("Tried to access flag arg, but flag '%s' does not accept any argument!\n", handler.current_flag->long_name);
		else
			printf("Tried to access flag arg, but flag '%c' does not accept any argument!\n", handler.current_flag->short_name);
		
		return NULL;
	}

	return handler.current_flag_arg;
}

static inline void clapMoveToNextCharOrNextArg(void)
{
	handler.current_arg_index++;
	if (handler.current_arg_index >= handler.current_arg_len)
	{
		handler.current_arg_index = 0;
		handler.arg_index++;
	}
}

uint8_t clapParse(int argc, char** argv)
{
	if (handler.arg_index >= argc)
		return CLAP_FALSE;

	handler.current_arg = argv[handler.arg_index];
	handler.current_arg_len = strlen(handler.current_arg);

	if (handler.current_arg_len < 2)
	{
		handler.current_flag = NULL;
		handler.current_non_opt = handler.current_arg;
		handler.arg_index++;
		return CLAP_TRUE;
	}

	if (!handler.next_is_no_opt && handler.current_arg[0] == '-')
	{
		handler.current_non_opt = NULL;

		if (handler.current_arg[1] == '-')
		{
			if (handler.current_arg_len == 2)
			{
				handler.next_is_no_opt = CLAP_TRUE;
				handler.current_flag = NULL;
				handler.arg_index++;
				return CLAP_TRUE;
			}

			const char* arg = strchr(handler.current_arg, '=');
			if (arg != NULL)
				arg++;

			for (size_t i = 0; i < handler.flag_count; i++)
			{
				CLAPflag* flag = &handler.flags[i];

				if (flag->long_name == CLAP_FLAG_NO_LONG)
					continue;

				if (!handler.subcommand && flag->subcommand_name)
					continue;

				if (handler.subcommand && !flag->subcommand_name)
					continue;

				if (handler.subcommand && strcmp(handler.subcommand->name, flag->subcommand_name) != 0)
					continue;

				if (strncmp(flag->long_name, &handler.current_arg[2], strlen(flag->long_name)) == 0)
				{
					handler.current_flag = flag;

					switch (flag->arg_type)
					{
						case CLAP_FLAG_NO_ARG:
						{
							if (arg && *arg != '\0')
							{
								printf("Parsed argument '%s' to flag '%s', but flag is of type CLAP_FLAG_NO_ARG!\n", arg, flag->long_name);
								handler.current_flag = NULL;
							}
							
							handler.current_flag_arg = NULL;
							
							break;
						}
						case CLAP_FLAG_OPT_ARG:
						{
							if (arg && *arg != '\0')
								handler.current_flag_arg = arg;
							else
								handler.current_flag_arg = NULL;

							break;
						}
						case CLAP_FLAG_REQ_ARG:
						{
							if (arg && *arg != '\0')
							{
								handler.current_flag_arg = arg;
								break;
							}
							
							if (handler.arg_index + 1 >= argc)
							{
								printf("Missing flag argument for flag '%s'!\n", flag->long_name);
								handler.current_flag = NULL;
								handler.arg_index++;
								break;
							}
							
							handler.current_flag_arg = argv[handler.arg_index + 1];
							handler.arg_index++;

							break;
						}
					}

					handler.arg_index++;
					return CLAP_TRUE;
				}
			}

			if (handler.subcommand)
				printf("Invalid subcommand flag '%s' specified for subcommand '%s'\n", &handler.current_arg[2], handler.subcommand->name);
			else
				printf("Invalid flag '%s' specified\n", &handler.current_arg[2]);

			handler.arg_index++;
			return CLAP_TRUE;
		}

		if (handler.current_arg_index == 0)
		{
			handler.current_flag = NULL;
			handler.current_arg_index++;
			return CLAP_TRUE;
		}

		const char flag_short_name = handler.current_arg[handler.current_arg_index];

		for (size_t i = 0; i < handler.flag_count; i++)
		{
			CLAPflag* flag = &handler.flags[i];

			if (flag->short_name == CLAP_FLAG_NO_SHORT)
				continue;

			if (!handler.subcommand && flag->subcommand_name)
				continue;

			if (handler.subcommand && !flag->subcommand_name)
				continue;

			if (handler.subcommand && strcmp(handler.subcommand->name, flag->subcommand_name) != 0)
				continue;

			if (flag->short_name == flag_short_name)
			{
				handler.current_flag = flag;

				switch (flag->arg_type)
				{
					case CLAP_FLAG_NO_ARG: clapMoveToNextCharOrNextArg(); break;
					case CLAP_FLAG_OPT_ARG:
					{
						handler.current_flag_arg = handler.current_arg_index == handler.current_arg_len - 1 ? NULL : &handler.current_arg[handler.current_arg_index + 1];
						handler.current_arg_index = 0;
						handler.arg_index++;
						break;
					}
					case CLAP_FLAG_REQ_ARG:
					{
						if (handler.current_arg_index == handler.current_arg_len - 1 && handler.arg_index + 1 >= argc)
						{
							printf("Missing flag argument for flag '%c'!\n", flag_short_name);
							handler.current_flag = NULL;
							handler.arg_index++;
							break;
						}

						if (handler.current_arg_index == handler.current_arg_len - 1)
						{
							handler.current_flag_arg = argv[handler.arg_index + 1];
							handler.arg_index++;
						}
						else
							handler.current_flag_arg = &handler.current_arg[handler.current_arg_index + 1];

						handler.current_arg_index = 0;
						handler.arg_index++;

						break;
					}
				}
				
				return CLAP_TRUE;
			}
		}

		if (handler.subcommand)
			printf("Invalid subcommand flag '%c' specified for subcommand '%s'\n", flag_short_name, handler.subcommand->name);
		else
			printf("Invalid flag '%c' specified\n", flag_short_name);

		handler.current_flag = NULL;
		clapMoveToNextCharOrNextArg();
		return CLAP_TRUE;
	}

	handler.next_is_no_opt = CLAP_FALSE;

	if (!handler.subcommand)
	{
		for (size_t i = 0; i < handler.subcommand_count; i++)
		{
			CLAPsubcommand* subcommand = &handler.subcommands[i];

			if (strcmp(subcommand->name, handler.current_arg) == 0)
			{
				handler.subcommand = subcommand;
				handler.current_flag = NULL;
				handler.current_non_opt = NULL;
				handler.arg_index++;
				return CLAP_TRUE;
			}
		}
	}

	handler.current_non_opt = handler.current_arg;
	handler.current_flag = NULL;
	handler.arg_index++;
	return CLAP_TRUE;
}

void clapResetFlags(void)
{
	handler.flag_count = 0;
}

void clapResetSubcommands(void)
{
	handler.subcommand_count = 0;
}

void clapResetParser(void)
{
	handler.arg_index = 0;
	handler.current_arg = NULL;
	handler.current_arg_index = 0;
	handler.current_flag = NULL;
	handler.current_flag_arg = NULL;
	handler.subcommand = NULL;
	handler.current_non_opt = NULL;
	handler.next_is_no_opt = CLAP_FALSE;
}

void clapReset(void)
{
	clapResetParser();
	clapResetSubcommands();
	clapResetFlags();
}

#endif
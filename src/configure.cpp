/** @file
	Adapted from:

    rtl_433, turns your Realtek RTL2832 based DVB dongle into a 433.92MHz generic data receiver.

    Copyright (C) 2012 by Benjamin Larsson <benjamin@southpole.se>

    Based on rtl_sdr
    Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdbool.h>

#include "configure.h"
#include "confparse.h"
#include "optparse.h"
#include "getopt.h"
#include "logwrap.h"
#include "compat_paths.h"

static CfgResult usage(int show_devs, CfgResult exit_code){
	Gui_fprintf(stderr,
		"Generic RF data receiver and decoder for ISM band devices using RTL-SDR and SoapySDR.\n"
		"\nUsage:\t= General options =\n"
		"\t[-V] Output the version string and exit\n"
        "\t[-v] Increase verbosity (can be used multiple times).\n"
        "\t\t -v : verbose, -vv : verbose decoders, -vvv : debug decoders, -vvvv : trace decoding).\n"
		"\t[-c <path>] Read config options from a file\n"
		"\t= Tuner options =\n"	
		"\t[-d <RTL-SDR USB device index> | :<RTL-SDR USB device serial> | <SoapySDR device query> | rtl_tcp]\n"
		"\t[-g <gain>] (default: auto)\n"
		"\t[-f <frequency>] [-f...] Receive frequency(s) (default: %i Hz)\n"
		"\t[-H <seconds>] Hop interval for polling of multiple frequencies (default: %i seconds)\n"
		"\t[-p <ppm_error] Correct rtl-sdr tuner frequency offset error (default: 0)\n"
		"\t[-s <sample rate>] Set sample rate (default: %i Hz)\n"
		"\t= Demodulator options =\n"
		"\t[-R <device>] Enable only the specified device decoding protocol (can be used multiple times)\n"
		"\t\t Specify a negative number to disable a device decoding protocol (can be used multiple times)\n"
		"\t[-G] Enable all device protocols, included those disabled by default\n"
		"\t[-X <spec> | help] Add a general purpose decoder (-R 0 to disable all other decoders)\n"
		"\t[-l <level>] Change detection level used to determine pulses [0-16384] (0 = auto) (default: %i)\n"
		"\t[-z <value>] Override short value in data decoder\n"
		"\t[-x <value>] Override long value in data decoder\n"
		"\t[-n <value>] Specify number of samples to take (each sample is 2 bytes: 1 each of I & Q)\n"
		"\t= Analyze/Debug options =\n"
		"\t[-a] Analyze mode. Print a textual description of the signal.\n"
		"\t[-A] Pulse Analyzer. Enable pulse analysis and decode attempt.\n"
		"\t\t Disable all decoders with -R 0 if you want analyzer output only.\n"
		"\t[-y <code>] Verify decoding of demodulated test data (e.g. \"{25}fb2dd58\") with enabled devices\n"
		"\t= File I/O options =\n"
        "\t[-S none|all|unknown|known] Signal auto save. Creates one file per signal.\n"
        "\t\t Note: Saves raw I/Q samples (uint8 pcm, 2 channel). Preferred mode for generating test files.\n"
		"\t[-r <filename>] Read data from input file instead of a receiver\n"
		"\t[-w <filename>] Save data stream to output file (a '-' dumps samples to stdout)\n"
		"\t[-W <filename>] Save data stream to output file, overwrite existing file\n"
		"\t= Data output options =\n"
		"\t[-F kv|json|csv|syslog|null] Produce decoded output in given format.\n"
		"\t\t Append output to file with :<filename> (e.g. -F csv:log.csv), defaults to stdout.\n"
		"\t\t Specify host/port for syslog with e.g. -F syslog:127.0.0.1:1514\n"
		"\t[-M time|reltime|notime|hires|utc|protocol|level|bits] Add various meta data to every output line.\n"
		"\t[-K FILE|PATH|<tag>] Add an expanded token or fixed tag to every output line.\n"
		"\t[-C native|si|customary] Convert units in decoded output.\n"
		"\t[-T <seconds>] Specify number of seconds to run\n"
		"\t[-E] Stop after outputting successful event(s)\n"
		"\t[-h] Output this usage help and exit\n"
		"\t\t Use -d, -g, -R, -X, -F, -M, -r, or -w without argument for more help\n\n",
		DEFAULT_FREQUENCY, DEFAULT_HOP_TIME, DEFAULT_SAMPLE_RATE, DEFAULT_LEVEL_LIMIT);

	if(show_devs){
		int num_r_devices = getDevCount();
		Gui_fprintf(stderr, "Supported device protocols:\n");
		for (int i = 0; i < num_r_devices; i++) {
			r_device *devptr = NULL;
			if (!getDev(i, &devptr)) {
				Gui_fprintf(stderr, "    [%02d] - error, could not access device information.\n", i + 1);
			}
			else {
				char disabledc = devptr->disabled ? '*' : ' ';
				if (devptr->disabled <= 2) // if not hidden
					Gui_fprintf(stderr, "    [%02d]%c %s\n", i + 1, disabledc, devptr->name);
			}
		}
		Gui_fprintf(stderr, "\n* Disabled by default, use -R n or -G\n");
	}

	return exit_code; // exit(exit_code); // handled at caller
}

static CfgResult help_device(void) {
	SdrDriverType drv = getDriverType();

	Gui_fprintf(stderr,
		"\tRTL-SDR device driver is %s.\n"
		"[-d <RTL-SDR USB device index>] (default: 0)\n"
		"[-d :<RTL-SDR USB device serial (can be set with rtl_eeprom -s)>]\n"
		"\tTo set gain for RTL-SDR use -g <gain> to set an overall gain in dB.\n"
		"\tSoapySDR device driver is %s.\n"
		"[-d \"\" Open default SoapySDR device\n"
		"[-d driver=rtlsdr Open e.g. specific SoapySDR device\n"
		"\tTo set gain for SoapySDR use -g ELEM=val,ELEM=val,... e.g. -g LNA=20,TIA=8,PGA=2 (for LimeSDR).\n"
		"[-d rtl_tcp[:host[:port]] (default: localhost:1234)\n"
		"\tSpecify host/port to connect to with e.g. -d rtl_tcp:127.0.0.1:1234\n", (drv == SDRDRV_RTLSDR ? "available" : "not available"), (drv == SDRDRV_SOAPYSDR ? "available" : "not available"));
	return CFG_EXITCODE_NULL; // exit(0); // handled at caller
}

static CfgResult help_flex() // copied from flex.c
{
	Gui_fprintf(stderr,
		"Use -X <spec> to add a flexible general purpose decoder.\n\n"
		"<spec> is \"name:modulation:short:long:reset[,key=value...]\"\n"
		"where:\n"
		"<name> can be any descriptive name tag you need in the output\n"
		"<modulation> is one of:\n"
		"\tOOK_MC_ZEROBIT :  Manchester Code with fixed leading zero bit\n"
		"\tOOK_PCM :         Pulse Code Modulation (RZ or NRZ)\n"
		"\tOOK_PPM :         Pulse Position Modulation\n"
		"\tOOK_PWM :         Pulse Width Modulation\n"
		"\tOOK_DMC :         Differential Manchester Code\n"
		"\tOOK_PIWM_RAW :    Raw Pulse Interval and Width Modulation\n"
		"\tOOK_PIWM_DC :     Differential Pulse Interval and Width Modulation\n"
		"\tOOK_MC_OSV1 :     Manchester Code for OSv1 devices\n"
		"\tFSK_PCM :         FSK Pulse Code Modulation\n"
		"\tFSK_PWM :         FSK Pulse Width Modulation\n"
		"\tFSK_MC_ZEROBIT :  Manchester Code with fixed leading zero bit\n"
		"<short>, <long>, and <reset> are the timings for the decoder in µs\n"
		"PCM     short: Nominal width of pulse [us]\n"
		"         long: Nominal width of bit period [us]\n"
		"PPM     short: Threshold between short and long gap [us]\n"
		"         long: Maximum gap size before new row of bits [us]\n"
		"PWM     short: Nominal width of '1' pulse [us]\n"
		"         long: Nominal width of '0' pulse [us]\n"
		"          gap: Maximum gap size before new row of bits [us]\n"
		"reset: Maximum gap size before End Of Message [us].\n"
		"for PWM use short:long:reset:gap[:tolerance[:syncwidth]]\n"
		"for DMC use short:long:reset:tolerance\n"
		"Available options are:\n"
		"\tdemod=<n> : the demod argument needed for some modulations\n"
		"\tbits=<n> : only match if at least one row has <n> bits\n"
		"\trows=<n> : only match if there are <n> rows\n"
		"\trepeats=<n> : only match if some row is repeated <n> times\n"
		"\t\tuse opt>=n to match at least <n> and opt<=n to match at most <n>\n"
		"\tinvert : invert all bits\n"
		"\treflect : reflect each byte (MSB first to MSB last)\n"
		"\tmatch=<bits> : only match if the <bits> are found\n"
		"\tpreamble=<bits> : match and align at the <bits> preamble\n"
		"\t\t<bits> is a row spec of {<bit count>}<bits as hex number>\n"
		"\tcountonly : suppress detailed row output\n\n"
		"E.g. -X \"doorbell:OOK_PWM_RAW:400:800:7000,match={24}0xa9878c,repeats>=3\"\n\n");
	return CFG_EXITCODE_NULL; // exit(0); // handled at caller
}

static CfgResult help_gain(void){
	Gui_fprintf(stderr,
		"-g <gain>] (default: auto)\n"
		"\tFor RTL-SDR: gain in dB (\"0\" is auto).\n"
		"\tFor SoapySDR: gain in dB for automatic distribution (\"\" is auto), or string of gain elements.\n"
		"\tE.g. \"LNA=20,TIA=8,PGA=2\" for LimeSDR.\n");
	return CFG_EXITCODE_NULL; // exit(0); // handled at caller
}

static CfgResult help_output(void){
	Gui_fprintf(stderr,
		"[-F kv|json|csv|syslog|null] Produce decoded output in given format.\n"
		"\tWithout this option the default is KV output. Use \"-F null\" to remove the default.\n"
		"\tAppend output to file with :<filename> (e.g. -F csv:log.csv), defaults to stdout.\n"
		"\tSpecify host/port for syslog with e.g. -F syslog:127.0.0.1:1514\n");
	return CFG_EXITCODE_NULL; //exit(0); // handled at caller
}

static CfgResult help_meta(void)
{
	Gui_fprintf(stderr,
            "[-M time|reltime|notime|hires|level] Add various metadata to every output line.\n"
            "\tUse \"time\" to add current date and time meta data (preset for live inputs).\n"
            "\tUse \"reltime\" to add sample position meta data (preset for read-file and stdin).\n"
            "\tUse \"notime\" to remove time meta data.\n"
            "\tUse \"hires\" to add microsecods to date time meta data.\n"
            "\tUse \"utc\" / \"noutc\" to output timestamps in UTC.\n"
            "\t\t(this may also be accomplished by invocation with TZ environment variable set).\n"
            "\tUse \"protocol\" / \"noprotocol\" to output the decoder protocol number meta data.\n"
            "\tUse \"level\" to add Modulation, Frequency, RSSI, SNR, and Noise meta data.\n"
            "\tUse \"bits\" to add bit representation to code outputs (for debug).\n");
	return CFG_EXITCODE_NULL; //exit(0); // handled at caller
}

static CfgResult help_read(void){
	Gui_fprintf(stderr,
		"[-r <filename>] Read data from input file instead of a receiver\n"
		"\tParameters are detected from the full path, file name, and extension.\n\n"
		"\tA center frequency is detected as (fractional) number suffixed with 'M',\n"
		"\t'Hz', 'kHz', 'MHz', or 'GHz'.\n\n"
		"\tA sample rate is detected as (fractional) number suffixed with 'k',\n"
		"\t'sps', 'ksps', 'Msps', or 'Gsps'.\n\n"
		"\tFile content and format are detected as parameters, possible options are:\n"
		"\t'cu8', 'cs16', 'cf32' ('IQ' implied), and 'am.s16'.\n\n"
		"\tParameters must be separated by non-alphanumeric chars and are case-insensitive.\n"
		"\tOverrides can be prefixed, separated by colon (':')\n\n"
		"\tE.g. default detection by extension: path/filename.am.s16\n"
		"\tforced overrides: am:s16:path/filename.ext\n");
	return CFG_EXITCODE_NULL; //exit(0); // handled at caller
}

static CfgResult help_write(void){
	Gui_fprintf(stderr,
		"[-w <filename>] Save data stream to output file (a '-' dumps samples to stdout)\n"
		"[-W <filename>] Save data stream to output file, overwrite existing file\n"
		"\tParameters are detected from the full path, file name, and extension.\n\n"
		"\tFile content and format are detected as parameters, possible options are:\n"
		"\t'cu8', 'cs16', 'cf32' ('IQ' implied),\n"
		"\t'am.s16', 'am.f32', 'fm.s16', 'fm.f32',\n"
		"\t'i.f32', 'q.f32', 'logic.u8', and 'vcd'.\n\n"
		"\tParameters must be separated by non-alphanumeric chars and are case-insensitive.\n"
		"\tOverrides can be prefixed, separated by colon (':')\n\n"
		"\tE.g. default detection by extension: path/filename.am.s16\n"
		"\tforced overrides: am:s16:path/filename.ext\n");
	return CFG_EXITCODE_NULL; //exit(0); // handled at caller
}

void add_infile(r_cfg_t *cfg, char *in_file)
{
	list_push(&cfg->in_files, strdup(in_file));
}

void clear_infiles(r_cfg_t *cfg) {
	list_free_elems(&cfg->in_files, free);
}

static int hasopt(int test, int argc, char *argv[], char const *optstring)
{
	int opt;
	optind = 1; // reset getopt
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		if (opt == test || optopt == test)
			return opt;
	}
	return 0;
}

static CfgResult parse_conf_option(r_cfg_t *cfg, int opt, char *arg);

#define OPTSTRING "hVvqDc:x:z:p:taAI:S:m:M:r:w:W:l:d:f:H:g:s:b:n:R:X:F:K:C:T:UGy:E"

// these should match the short options exactly
static struct conf_keywords const conf_keywords[] = {
	{ "help", 'h' },
	{ "verbose", 'v' },
	{ "version", 'V' },
	{ "config_file", 'c' },
	{ "report_meta", 'M' },
	{ "device", 'd' },
	{ "gain", 'g' },
	{ "frequency", 'f' },
	{ "hop_interval", 'H' },
	{ "ppm_error", 'p' },
	{ "sample_rate", 's' },
	{ "protocol", 'R' },
	{ "decoder", 'X' },
	{ "register_all", 'G' },
	{ "out_block_size", 'b' },
	{ "level_limit", 'l' },
	{ "samples_to_read", 'n' },
	{ "analyze", 'a' },
	{ "analyze_pulses", 'A' },
	{ "include_only", 'I' },
	{ "read_file", 'r' },
	{ "write_file", 'w' },
	{ "overwrite_file", 'W' },
	{ "signal_grabber", 'S' },
	{ "override_short", 'z' },
	{ "override_long", 'x' },
	{ "output", 'F' },
	{ "output_tag", 'K' },
	{ "convert", 'C' },
	{ "duration", 'T' },
	{ "test_data", 'y' },
	{ "stop_after_successful_events", 'E' },
	{ NULL }
};

static CfgResult parse_conf_text(r_cfg_t *cfg, char *conf)
{
	CfgResult r = CFG_SUCCESS_GO_ON;

	int opt;
	char *arg;
	char *p = conf;
	if (!conf || !*conf)
		return r;
	while ((opt = getconf(&p, conf_keywords, &arg)) != -1 && r == CFG_SUCCESS_GO_ON) {
		r = parse_conf_option(cfg, opt, arg);
	}

	return r;
}

static CfgResult parse_conf_file(r_cfg_t *cfg, char const *path)
{
	CfgResult r = CFG_EXITCODE_MINUS1;

	if (!path || !*path || !strcmp(path, "null") || !strcmp(path, "0"))
		return r;
	char *conf = readconf(path);
	r = parse_conf_text(cfg, conf);
	free(conf);
	return r;
}

static CfgResult parse_conf_try_default_files(r_cfg_t *cfg)
{
	CfgResult r = CFG_SUCCESS_GO_ON;

    char **paths = compat_get_default_conf_paths();
    for (int a = 0; paths[a]; a++) {
		Gui_fprintf(stderr, "Trying conf file at \"%s\"...\n", paths[a]);
        if (hasconf(paths[a])) {
			Gui_fprintf(stderr, "Reading conf from \"%s\".\n", paths[a]);
            r = parse_conf_file(cfg, paths[a]);
			break;
        }
    }

	return r;
}

static int f_null_used;

static CfgResult parse_conf_args(r_cfg_t *cfg, int argc, char *argv[])
{
	if (!cfg) {
		Gui_fprintf(stderr, "Internal error, no r_cfg_t object specified.\n");
		return CFG_EXITCODE_MINUS1;
	}

	CfgResult r = CFG_SUCCESS_GO_ON;

	f_null_used = 0;

	int opt;
	optind = 1; // reset getopt
	while ((opt = getopt(argc, argv, OPTSTRING)) != -1) {
		if (opt == '?')
			opt = optopt; // allow missing arguments
		r = parse_conf_option(cfg, opt, optarg);
		if (r != CFG_SUCCESS_GO_ON) break;
	}

//	Not in GUI-based programs:
//	// restore old behavior: if no output is specified and NULL option is not used, enforce key-value mode on stdout
//	if (!cfg->outputs_configured && !f_null_used) {
//		cfg->output_path_kv[0] = 0;
//		cfg->outputs_configured |= OUTPUT_KV;
//	}

	return r;
}

static CfgResult parse_conf_option(r_cfg_t *cfg, int opt, char *arg) {
	if (!cfg) {
		Gui_fprintf(stderr, "Internal error, no r_cfg_t object specified.\n");
		return CFG_EXITCODE_MINUS1;
	}

    if (arg && (!strcmp(arg, "help") || !strcmp(arg, "?"))) {
        arg = NULL; // remove the arg if it's a request for the usage help
    }
    switch (opt) {
    case 'h':
		return usage(0, CFG_EXITCODE_NULL);
    case 'V':
		return CFG_EXITCODE_NULL; // we already printed the version
    case 'v':
        if (!arg)
            cfg->verbosity++;
        else
            cfg->verbosity = atobv(arg, 1);
        break;
	case 'c':
        return parse_conf_file(cfg, arg);
	case 'd':
        if (!arg)
            return help_device();
		strcpy_s(cfg->dev_query, sizeof(cfg->dev_query), arg);
		break;
    case 'f':
        if (cfg->frequencies < MAX_FREQS)
            cfg->frequency[cfg->frequencies++] = atouint32_metric(arg, "-f: ");
        else
			Gui_fprintf(stderr, "Max number of frequencies reached %d\n", MAX_FREQS);
		break;
    case 'H':
		cfg->hop_time = atoi_time(arg, "-H: ");
		break;
	case 'g':
		if (!arg)
			return help_gain();
		strcpy_s(cfg->gain_str, sizeof(cfg->gain_str), arg);
		break;
	case 'G':
		if (atobv(arg, 1)) {
			int devcnt = getDevCount();
			list_ensure_size(&cfg->active_prots, devcnt);
			for (int a = 0; a < cfg->active_prots.len; a++) {
				cfg->active_prots.elems[a] = (void*) 1; // 1 is also suitable for devices having disabled==2 (since in librtl_433 we do not look at prots_acted for such devices)
			}
			while (cfg->active_prots.len < devcnt) list_push(&cfg->active_prots, (void*) 1);
		}
		break;
	case 'p':
        cfg->ppm_error = atobv(arg, 0);
        break;
    case 's':
        cfg->samp_rate = atouint32_metric(arg, "-s: ");
        break;
    case 'b':{
		uint32_t val = atouint32_metric(arg, "-b: ");
		if (val % 512) {
			val -= (val % 512);
			if (!val) val = 512;
			Gui_fprintf(stderr, "-b param must be multiple of 512. Falling back to %d\n", val);
		}
		cfg->out_block_size = val;
		break;
	}
    case 'l':
        cfg->level_limit = atouint32_metric(arg, "-l: ");
        break;
    case 'n':
        cfg->bytes_to_read = atouint32_metric(arg, "-n: ") * 2;
        break;
    case 'a':
		if (atobv(arg, 1)){
			cfg->analyze_am = 1;
		}
		break;
	case 'A':
		if (atobv(arg, 1)) {
			cfg->analyze_pulses = 1;
		}
		break;
	case 'I':
		Gui_fprintf(stderr, "include_only (-I) is deprecated. Use -S none|all|unknown|known\n");
		return CFG_EXITCODE_ONE;
    case 'r':
        if (!arg)
            return help_read();
		add_infile(cfg, arg);
		// TODO: check_read_file_info()
        break;
	case 'w':
	case 'W': {
		if (!arg)
			return help_write();
		//file_info_t fi;
		//parse_file_info(arg, &fi);
		//if (!check_write_file_info(&fi)) {
		//	Gui_fprintf(stderr, "File type not supported as output (%s).\n", fi.spec);
		//	return CFG_EXITCODE_ONE;
		//}
		//w: rtl->add_dumper(arg, demod->dumper, 0); W: rtl->add_dumper(arg, demod->dumper, 1);
		strcpy_s(cfg->out_filename, sizeof(cfg->out_filename), arg);
		if (opt == 'W') cfg->overwrite_modes |= OVR_SUBJ_SAMPLES;
		break;
	}
    case 't':
		Gui_fprintf(stderr, "test_mode (-t) is deprecated. Use -S none|all|unknown|known\n");
		return CFG_EXITCODE_ONE;
	case 'S':
		if (strcasecmp(arg, "all") == 0)
			cfg->grab_mode = GRAB_ALL_DEVICES;
		else if (strcasecmp(arg, "unknown") == 0)
			cfg->grab_mode = GRAB_UNKNOWN_DEVICES;
		else if (strcasecmp(arg, "known") == 0)
			cfg->grab_mode = GRAB_KNOWN_DEVICES;
		else
			cfg->grab_mode = (GrabMode) atobv(arg, 1);
		if (cfg->grab_mode) {
			cfg->output_path_sigdmp[0] = 0; // "" -> working directory. Specifying other target directories is not yet supported via command line (same as in rtl_433)
		}
		break;
	case 'm':
		Gui_fprintf(stderr, "sample mode option is deprecated.\n");
        return usage(0, CFG_EXITCODE_ONE);
    case 'M':
		if (!arg)
			return help_meta();

        if (!strcasecmp(arg, "time"))
            cfg->report_time_preference = REPORT_TIME_DATE;
        else if (!strcasecmp(arg, "reltime"))
            cfg->report_time_preference = REPORT_TIME_SAMPLES;
        else if (!strcasecmp(arg, "notime"))
            cfg->report_time_preference = REPORT_TIME_OFF;
        else if (!strcasecmp(arg, "hires"))
            cfg->report_time_hires = 1;
        else if (!strcasecmp(arg, "utc"))
            cfg->report_time_utc = 1;
        else if (!strcasecmp(arg, "noutc"))
            cfg->report_time_utc = 0;
        else if (!strcasecmp(arg, "protocol"))
            cfg->report_protocol = 1;
        else if (!strcasecmp(arg, "noprotocol"))
            cfg->report_protocol = 0;
        else if (!strcasecmp(arg, "level"))
			cfg->report_meta = 1;
        else if (!strcasecmp(arg, "bits"))
            cfg->verbose_bits = 1;
        else if (!strcasecmp(arg, "description"))
            cfg->report_description = 1;
		else
			cfg->report_meta = atobv(arg, 1);
		break;
    case 'D':
		Gui_fprintf(stderr, "debug option (-D) is deprecated. See -v to increase verbosity\n");
		break;
	case 'z':
		cfg->override_short = atoi(arg);
		break;
	case 'x':
		cfg->override_long = atoi(arg);
		break;
	case 'R': {
		if(!arg)
			return usage(0, CFG_EXITCODE_NULL);

		int num_r_devices = getDevCount();

		int n = atoi(arg);
		if (n > num_r_devices || -n > num_r_devices) {
			Gui_fprintf(stderr, "Remote device number specified larger than number of devices\n\n");
			usage(1, CFG_EXITCODE_ONE);
		}
		r_device *dev;
		if((n > 0 && getDev(n, &dev)>0 && dev->disabled > 2) || (n < 0 && getDev(-n, &dev) >= 0 && dev->disabled > 2)) {
			Gui_fprintf(stderr, "Remote device number specified is invalid\n\n");
			usage(1, CFG_EXITCODE_ONE);
		}

		if (n == 0 || (n > 0 && !cfg->active_prots.len)) {
			list_ensure_size(&cfg->active_prots, num_r_devices);
			for (int a = 0; a < cfg->active_prots.len; a++) {
				cfg->active_prots.elems[a] = 0;
			}
			while (cfg->active_prots.len < num_r_devices) list_push(&cfg->active_prots, 0);
		}

		if (n >= 1) {
			if(n <= cfg->active_prots.len) cfg->active_prots.elems[n - 1] = (void*) 1;
		}
		else if (n <= -1) {
			if ((-n) <= cfg->active_prots.len) cfg->active_prots.elems[-n - 1] = 0;
		}
		else { // n == 0
			Gui_fprintf(stderr, "Disabling all device decoders.\n");
		}
		break;
	}

	case 'X':
		list_push(&cfg->flex_specs, strdup(arg));
		break;
	case 'q':
		Gui_fprintf(stderr, "quiet option (-q) is default and deprecated. See -v to increase verbosity\n");
		break;
	case 'F': {
		if (!arg)
			return help_output();
		char *out = arg_param(arg);
		if (strncmp(arg, "json", 4) == 0) {
			strcpy_s(cfg->output_path_json, sizeof(cfg->output_path_json),(out ? out : "")); // empty string forces output to stdout
			cfg->outputs_configured |= OUTPUT_JSON;
		}
		else if (strncmp(arg, "csv", 3) == 0) {
			strcpy_s(cfg->output_path_csv, sizeof(cfg->output_path_csv), (out ? out : "")); // empty string forces output to stdout
			cfg->outputs_configured |= OUTPUT_CSV;
		}
		else if (strncmp(arg, "kv", 2) == 0) {
			strcpy_s(cfg->output_path_kv, sizeof(cfg->output_path_kv), (out ? out : "")); // empty string forces output to stdout
			cfg->outputs_configured |= OUTPUT_KV;
		}
		else if (strncmp(arg, "syslog", 6) == 0) {
			char *host = "localhost";
			char *port = "514";
			if (hostport_param(arg_param(arg), &host, &port)) {
				Gui_fprintf(stderr, "Syslog UDP datagrams to %s port %s\n", host, port);
				strcpy_s(cfg->output_udp_host, sizeof(cfg->output_udp_host), host);
				strcpy_s(cfg->output_udp_port, sizeof(cfg->output_udp_port), port);
			}
			else {
				Gui_fprintf(stderr, "Malformed Ipv6 address!\n");
				return CFG_EXITCODE_ONE;
			}
		}
		else if (strncmp(arg, "null", 4) == 0) {
			f_null_used = 1;
		}
		else {
			Gui_fprintf(stderr, "Invalid output format %s\n", arg);
			return usage(0, CFG_EXITCODE_ONE);
		}
		break;
	}
    case 'K':
        cfg->output_tag = arg;
        break;
	case 'C':
        if (strcmp(arg, "native") == 0) {
            cfg->conversion_mode = CONVERT_NATIVE;
        }
        else if (strcmp(arg, "si") == 0) {
            cfg->conversion_mode = CONVERT_SI;
        }
        else if (strcmp(arg, "customary") == 0) {
            cfg->conversion_mode = CONVERT_CUSTOMARY;
        }
        else {
			Gui_fprintf(stderr, "Invalid conversion mode %s\n", arg);
            return usage(0, CFG_EXITCODE_ONE);
        }
        break;
	case 'U': {
		Gui_fprintf(stderr, "UTC mode option (-U) is deprecated. Please use \"-M utc\".\n");
		return CFG_EXITCODE_ONE; // exit(1);
	}
    case 'T':
		cfg->duration = atoi_time(arg, "-T: ");
		if (cfg->duration < 1) {
			Gui_fprintf(stderr, "Duration '%s' not a positive number; will continue indefinitely\n", arg);
		}
		break;
    case 'y':
		strcpy_s(cfg->test_data, sizeof(cfg->test_data), arg);
		break;
	case 'E':
		cfg->stop_after_successful_events_flag = atobv(arg, 1);
		break;
    default:
        return usage(0, CFG_EXITCODE_ONE);
    }
	return CFG_SUCCESS_GO_ON;
}

CfgResult configure_librtl433(r_cfg_t *cfg, int argc, char **argv, int allow_default_cfgfile) {
	CfgResult r;

	// Look for default config
	if (allow_default_cfgfile && !hasopt('c', argc, argv, OPTSTRING)) { // if there is no explicit conf file option look for default conf files
		parse_conf_try_default_files(cfg);
	}

	// Evaluate command line and (optional) non-default config
	r = parse_conf_args(cfg, argc, argv);

	// add all remaining positional arguments as input files
	while (argc > optind) {
		add_infile(cfg, argv[optind++]);
	}

	return r;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *              Definition of entries in the RX list               *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include <memory.h>
#include <malloc.h>

#include "rxlist_entry.h"

rx_entry::rx_entry() {
	this->devid = -1;
	memset(this->time, 0, sizeof(this->time));
	memset(this->type, 0, sizeof(this->type));
	memset(this->model, 0, sizeof(this->model));
	this->pdat.sample_rate = 0;
	this->pdat.num_pulses = 0;
	this->pdat.pulse = NULL;
	this->pdat.gap = NULL;
	this->pdat.freq1_hz = 0;
	this->pdat.freq2_hz = 0;
	this->pdat.rssi_db = 0;
	this->pdat.snr_db = 0;
	this->pdat.noise_db = 0;
	this->pdat.segment_startidx = 0;
	this->pdat.segment_len = 0;
	this->pdat.num_samples = 0;
	this->mod = SIGNAL_MODULATION_UNK; // todo: move to pdat level?
	this->data = NULL;
}

rx_entry::~rx_entry() {
	if (this->pdat.pulse) free(this->pdat.pulse);
	// if (this->pdat.gap) free(this->pdat.gap); gap is freed with the previous line (it's just a pointer into the region alloced for pulse )
	if (this->data) data_free(this->data); // frees the list (effectively, it decrements the usage counter and only actually frees the memory if it becomes 0 again)
}

int rx_entry::copyData(data_t *dat) {
	if (this->data)
		return 0;

	this->data = data_retain(dat); // Copies list by just saving the pointer and increasing the usage pointer
	return 1;
}

data_t *rx_entry::getData() {
	return this->data;
}

int rx_entry::copyPulses(const pulse_data_t *pulses, unsigned segment_start, unsigned segment_l) {
	if (!pulses) return 0;
	// check if pulses are already taken (currently calling this function is allowed once per instance. might be changed if necessary...)
	if (this->pdat.num_pulses || this->pdat.num_samples || this->pdat.sample_rate) return 0;
	// check pulse indices
	if ((segment_start + segment_l) > pulses->num_pulses) return 0;
	// alloc memory for pulses
	this->pdat.pulse = (int*) malloc(2* pulses->num_pulses * sizeof(int));
	if (!this->pdat.pulse) return 0;
	this->pdat.gap = &this->pdat.pulse[pulses->num_pulses]; // gap is a pointer to the start of the second half of the same array
	// copy pulses
	this->pdat.sample_rate = pulses->sample_rate;
	this->pdat.num_pulses = pulses->num_pulses;
	this->pdat.freq1_hz = pulses->freq1_hz;
	this->pdat.freq2_hz = pulses->freq2_hz;
	this->pdat.rssi_db = pulses->rssi_db;
	this->pdat.snr_db = pulses->snr_db;
	this->pdat.noise_db = pulses->noise_db;
	this->pdat.segment_startidx = segment_start;
	this->pdat.segment_len = segment_l;
	memcpy(this->pdat.pulse, pulses->pulse, pulses->num_pulses * sizeof(int));
	memcpy(this->pdat.gap, pulses->gap, pulses->num_pulses * sizeof(int));
	// pre-calculate the total number of samples
	for (uint32_t a = 0; a < (pulses->num_pulses -1); a++) { // for all except the last pair: add up pulse and gap samples
		this->pdat.num_samples += this->pdat.pulse[a];
		this->pdat.num_samples += this->pdat.gap[a];
	}
	this->pdat.num_samples += this->pdat.pulse[pulses->num_pulses - 1]; // last pair: add up all pulse samples
	int last_gap_maxdsp = (this->pdat.num_samples / 100)+1; // don't display too much of the last gap. 1% of total signal length +1 should be enough to draw the falling edge.
	this->pdat.num_samples += min(this->pdat.gap[pulses->num_pulses - 1], last_gap_maxdsp); // (the reduced num_samples only affects the visualisation since only the last gap is shortened)
	return 1;
}

pPulseDatCompact rx_entry::getPulses() {
	return &this->pdat;
}

int rx_entry::getDevId() {
	return this->devid;
}

char *rx_entry::getTime() {
	return this->time;
}

char *rx_entry::getType() {
	return this->type;
}

char *rx_entry::getModel() {
	return this->model;
}

void rx_entry::setDevId(int v) {
	this->devid = v;
}

void rx_entry::setModulation(sigmod sm) {
	this->mod = sm;
}

sigmod rx_entry::getModulation() {
	return this->mod;
}

void rx_entry::setTime(char *str) {
	strncpy_s(this->time, sizeof(this->time), str, sizeof(this->time));
	this->time[sizeof(this->time) - 1] = 0;
}

void rx_entry::setType(char *str) {
	strncpy_s(this->type, str, sizeof(this->type));
	this->type[sizeof(this->type) - 1] = 0;
}

void rx_entry::setModel(char *str) {
	strncpy_s(this->model, sizeof(this->model), str, sizeof(this->model));
	this->model[sizeof(this->model) - 1] = 0;
}

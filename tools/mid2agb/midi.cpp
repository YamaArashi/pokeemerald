// Copyright(c) 2016 YamaArashi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include "midi.h"
#include "main.h"
#include "error.h"
#include "agb.h"
#include "tables.h"

enum class MidiEventCategory
{
    Control,
    SysEx,
    Meta,
    Invalid,
};

MidiFormat g_midiFormat;
std::int_fast32_t g_midiTrackCount;
std::int16_t g_midiTimeDiv;

int g_midiChan;
std::int32_t g_initialWait;

static long s_trackDataStart;
static std::vector<Event> s_seqEvents;
static std::vector<Event> s_trackEvents;
static std::int32_t s_absoluteTime;
static int s_blockCount = 0;
static int s_minNote;
static int s_maxNote;
static int s_runningStatus;

void Seek(long offset)
{
    if (std::fseek(g_inputFile, offset, SEEK_SET) != 0)
        RaiseError("failed to seek to %l", offset);
}

void Skip(long offset)
{
    if (std::fseek(g_inputFile, offset, SEEK_CUR) != 0)
        RaiseError("failed to skip %l bytes", offset);
}

std::string ReadSignature()
{
    char signature[4];

    if (std::fread(signature, 4, 1, g_inputFile) != 1)
        RaiseError("failed to read signature");

    return std::string(signature, 4);
}

std::uint32_t ReadInt8()
{
    int c = std::fgetc(g_inputFile);

    if (c < 0)
        RaiseError("unexpected EOF");

    return c;
}

std::uint32_t ReadInt16()
{
    std::uint32_t val = 0;
    val |= ReadInt8() << 8;
    val |= ReadInt8();
    return val;
}

std::uint32_t ReadInt24()
{
    std::uint32_t val = 0;
    val |= ReadInt8() << 16;
    val |= ReadInt8() << 8;
    val |= ReadInt8();
    return val;
}

std::uint32_t ReadInt32()
{
    std::uint32_t val = 0;
    val |= ReadInt8() << 24;
    val |= ReadInt8() << 16;
    val |= ReadInt8() << 8;
    val |= ReadInt8();
    return val;
}

std::uint32_t ReadVLQ()
{
    std::uint32_t val = 0;
    std::uint32_t c;

    do
    {
        c = ReadInt8();
        val <<= 7;
        val |= (c & 0x7F);
    } while (c & 0x80);

    return val;
}

void ReadMidiFileHeader()
{
    Seek(0);

    if (ReadSignature() != "MThd")
        RaiseError("MIDI file header signature didn't match \"MThd\"");

    std::uint32_t headerLength = ReadInt32();

    if (headerLength != 6)
        RaiseError("MIDI file header length isn't 6");

    std::uint16_t midiFormat = ReadInt16();

    if (midiFormat >= 2)
        RaiseError("unsupported MIDI format (%u)", midiFormat);

    g_midiFormat = (MidiFormat)midiFormat;
    g_midiTrackCount = ReadInt16();
    g_midiTimeDiv = ReadInt16();

    if (g_midiTimeDiv < 0)
        RaiseError("unsupported MIDI time division (%d)", g_midiTimeDiv);
}

long ReadMidiTrackHeader(long offset)
{
    Seek(offset);

    if (ReadSignature() != "MTrk")
        RaiseError("MIDI track header signature didn't match \"MTrk\"");

    long size = ReadInt32();

    s_trackDataStart = std::ftell(g_inputFile);

    return size + 8;
}

void StartTrack()
{
    Seek(s_trackDataStart);
    s_absoluteTime = 0;
    s_runningStatus = 0;
}

void SkipEventData()
{
    Skip(ReadVLQ());
}

void DetermineEventCategory(MidiEventCategory& category, int& typeChan, int& size)
{
    typeChan = ReadInt8();

    if (typeChan < 0x80)
    {
        // If data byte was found, use the running status.
        ungetc(typeChan, g_inputFile);
        typeChan = s_runningStatus;
    }

    if (typeChan == 0xFF)
    {
        category = MidiEventCategory::Meta;
        size = 0;
        s_runningStatus = 0;
    }
    else if (typeChan >= 0xF0)
    {
        category = MidiEventCategory::SysEx;
        size = 0;
        s_runningStatus = 0;
    }
    else if (typeChan >= 0x80)
    {
        category = MidiEventCategory::Control;

        switch (typeChan >> 4)
        {
        case 0xC:
        case 0xD:
            size = 1;
            break;
        default:
            size = 2;
            break;
        }
        s_runningStatus = typeChan;
    }
    else
    {
        category = MidiEventCategory::Invalid;
    }
}

void MakeBlockEvent(Event& event, EventType type)
{
    event.type = type;
    event.param1 = s_blockCount++;
    event.param2 = 0;
}

std::string ReadEventText()
{
    char buffer[9];
    std::uint32_t length = ReadVLQ();

    if (length <= 9)
    {
        if (fread(buffer, length, 1, g_inputFile) != 1)
            RaiseError("failed to read event text");
    }
    else
    {
        Skip(length);
        length = 0;
    }

    return std::string(buffer, length);
}

bool ReadSeqEvent(Event& event)
{
    s_absoluteTime += ReadVLQ();
    event.time = s_absoluteTime;

    MidiEventCategory category;
    int typeChan;
    int size;

    DetermineEventCategory(category, typeChan, size);

    if (category == MidiEventCategory::Control)
    {
        Skip(size);
        return false;
    }

    if (category == MidiEventCategory::SysEx)
    {
        SkipEventData();
        return false;
    }

    if (category == MidiEventCategory::Invalid)
        RaiseError("invalid event");

    // meta event
    int metaEventType = ReadInt8();

    if (metaEventType >= 1 && metaEventType <= 7)
    {
        // text event
        std::string text = ReadEventText();

        if (text == "[" || text == "loopStart")
            MakeBlockEvent(event, EventType::LoopBegin);
        else if (text == "][")
            MakeBlockEvent(event, EventType::LoopEndBegin);
        else if (text == "]" || text == "loopEnd")
            MakeBlockEvent(event, EventType::LoopEnd);
        else if (text == ":")
            MakeBlockEvent(event, EventType::Label);
        else
            return false;
    }
    else
    {
        switch (metaEventType)
        {
        case 0x2F: // end of track
            SkipEventData();
            event.type = EventType::EndOfTrack;
            event.param1 = 0;
            event.param2 = 0;
            break;
        case 0x51: // tempo
            if (ReadVLQ() != 3)
                RaiseError("invalid tempo size");

            event.type = EventType::Tempo;
            event.param1 = 0;
            event.param2 = ReadInt24();
            break;
        case 0x58: // time signature
        {
            if (ReadVLQ() != 4)
                RaiseError("invalid time signature size");

            int numerator = ReadInt8();
            int denominatorExponent = ReadInt8();

            if (denominatorExponent >= 16)
                RaiseError("invalid time signature denominator");

            Skip(2); // ignore other values

            int clockTicks = 96 * numerator * g_clocksPerBeat;
            int denominator = 1 << denominatorExponent;
            int timeSig = clockTicks / denominator;

            if (timeSig <= 0 || timeSig >= 0x10000)
                RaiseError("invalid time signature");

            event.type = EventType::TimeSignature;
            event.param1 = 0;
            event.param2 = timeSig;
            break;
        }
        default:
            SkipEventData();
            return false;
        }
    }

    return true;
}

void ReadSeqEvents()
{
    StartTrack();

    for (;;)
    {
        Event event = {};

        if (ReadSeqEvent(event))
        {
            s_seqEvents.push_back(event);

            if (event.type == EventType::EndOfTrack)
                return;
        }
    }
}

bool CheckNoteEnd(Event& event)
{
    event.param2 += ReadVLQ();

    MidiEventCategory category;
    int typeChan;
    int size;

    DetermineEventCategory(category, typeChan, size);

    if (category == MidiEventCategory::Control)
    {
        int chan = typeChan & 0xF;

        if (chan != g_midiChan)
        {
            Skip(size);
            return false;
        }

        switch (typeChan & 0xF0)
        {
        case 0x80: // note off
        {
            int note = ReadInt8();
            ReadInt8(); // ignore velocity
            if (note == event.note)
                return true;
            break;
        }
        case 0x90: // note on
        {
            int note = ReadInt8();
            int velocity = ReadInt8();
            if (velocity == 0 && note == event.note)
                return true;
            break;
        }
        default:
            Skip(size);
            break;
        }

        return false;
    }

    if (category == MidiEventCategory::SysEx)
    {
        SkipEventData();
        return false;
    }

    if (category == MidiEventCategory::Meta)
    {
        int metaEventType = ReadInt8();
        SkipEventData();

        if (metaEventType == 0x2F)
            RaiseError("note doesn't end");

        return false;
    }

    RaiseError("invalid event");
}

void FindNoteEnd(Event& event)
{
    // Save the current file position and running status
    // which get modified by CheckNoteEnd.
    long startPos = ftell(g_inputFile);
    int savedRunningStatus = s_runningStatus;

    event.param2 = 0;

    while (!CheckNoteEnd(event))
        ;

    Seek(startPos);
    s_runningStatus = savedRunningStatus;
}

bool ReadTrackEvent(Event& event)
{
    s_absoluteTime += ReadVLQ();
    event.time = s_absoluteTime;

    MidiEventCategory category;
    int typeChan;
    int size;

    DetermineEventCategory(category, typeChan, size);

    if (category == MidiEventCategory::Control)
    {
        int chan = typeChan & 0xF;

        if (chan != g_midiChan)
        {
            Skip(size);
            return false;
        }

        switch (typeChan & 0xF0)
        {
        case 0x90: // note on
        {
            int note = ReadInt8();
            int velocity = ReadInt8();

            if (velocity != 0)
            {
                event.type = EventType::Note;
                event.note = note;
                event.param1 = velocity;
                FindNoteEnd(event);
                if (event.param2 > 0)
                {
                    if (note < s_minNote)
                        s_minNote = note;
                    if (note > s_maxNote)
                        s_maxNote = note;
                }
            }
            break;
        }
        case 0xB0: // controller event
            event.type = EventType::Controller;
            event.param1 = ReadInt8(); // controller index
            event.param2 = ReadInt8(); // value
            break;
        case 0xC0: // instrument change
            event.type = EventType::InstrumentChange;
            event.param1 = ReadInt8(); // instrument
            event.param2 = 0;
            break;
        case 0xE0: // pitch bend
            event.type = EventType::PitchBend;
            event.param1 = ReadInt8();
            event.param2 = ReadInt8();
            break;
        default:
            Skip(size);
            return false;
        }

        return true;
    }

    if (category == MidiEventCategory::SysEx)
    {
        SkipEventData();
        return false;
    }

    if (category == MidiEventCategory::Meta)
    {
        int metaEventType = ReadInt8();
        SkipEventData();

        if (metaEventType == 0x2F)
        {
            event.type = EventType::EndOfTrack;
            event.param1 = 0;
            event.param2 = 0;
            return true;
        }

        return false;
    }

    RaiseError("invalid event");
}

void ReadTrackEvents()
{
    StartTrack();

    s_trackEvents.clear();

    s_minNote = 0xFF;
    s_maxNote = 0;

    for (;;)
    {
        Event event = {};

        if (ReadTrackEvent(event))
        {
            s_trackEvents.push_back(event);

            if (event.type == EventType::EndOfTrack)
                return;
        }
    }
}

bool EventCompare(const Event& event1, const Event& event2)
{
    if (event1.time < event2.time)
        return true;

    if (event1.time > event2.time)
        return false;

    unsigned event1Type = (unsigned)event1.type;
    unsigned event2Type = (unsigned)event2.type;

    if (event1.type == EventType::Note)
        event1Type += event1.note;

    if (event2.type == EventType::Note)
        event2Type += event2.note;

    if (event1Type < event2Type)
        return true;

    if (event1Type > event2Type)
        return false;

    if (event1.type == EventType::EndOfTie)
    {
        if (event1.note < event2.note)
            return true;

        if (event1.note > event2.note)
            return false;
    }

    return false;
}

std::unique_ptr<std::vector<Event>> MergeEvents()
{
    std::unique_ptr<std::vector<Event>> events(new std::vector<Event>());

    unsigned trackEventPos = 0;
    unsigned seqEventPos = 0;

    while (s_trackEvents[trackEventPos].type != EventType::EndOfTrack
        && s_seqEvents[seqEventPos].type != EventType::EndOfTrack)
    {
        if (EventCompare(s_trackEvents[trackEventPos], s_seqEvents[seqEventPos]))
            events->push_back(s_trackEvents[trackEventPos++]);
        else
            events->push_back(s_seqEvents[seqEventPos++]);
    }

    while (s_trackEvents[trackEventPos].type != EventType::EndOfTrack)
        events->push_back(s_trackEvents[trackEventPos++]);

    while (s_seqEvents[seqEventPos].type != EventType::EndOfTrack)
        events->push_back(s_seqEvents[seqEventPos++]);

    // Push the EndOfTrack event with the larger time.
    if (EventCompare(s_trackEvents[trackEventPos], s_seqEvents[seqEventPos]))
        events->push_back(s_seqEvents[seqEventPos]);
    else
        events->push_back(s_trackEvents[trackEventPos]);

    return events;
}

void ConvertTimes(std::vector<Event>& events)
{
    for (Event& event : events)
    {
        event.time = (24 * g_clocksPerBeat * event.time) / g_midiTimeDiv;

        if (event.type == EventType::Note)
        {
            event.param1 = g_noteVelocityLUT[event.param1];

            std::uint32_t duration = (24 * g_clocksPerBeat * event.param2) / g_midiTimeDiv;

            if (duration == 0)
                duration = 1;

            if (!g_exactGateTime && duration < 96)
                duration = g_noteDurationLUT[duration];

            event.param2 = duration;
        }
    }
}

std::unique_ptr<std::vector<Event>> InsertTimingEvents(std::vector<Event>& inEvents)
{
    std::unique_ptr<std::vector<Event>> outEvents(new std::vector<Event>());

    Event timingEvent = {};
    timingEvent.time = 0;
    timingEvent.type = EventType::TimeSignature;
    timingEvent.param2 = 96 * g_clocksPerBeat;

    for (const Event& event : inEvents)
    {
        while (EventCompare(timingEvent, event))
        {
            outEvents->push_back(timingEvent);
            timingEvent.time += timingEvent.param2;
        }

        if (event.type == EventType::TimeSignature)
        {
            if (g_agbTrack == 1 && event.param2 != timingEvent.param2)
            {
                Event originalTimingEvent = event;
                originalTimingEvent.type = EventType::OriginalTimeSignature;
                outEvents->push_back(originalTimingEvent);
            }
            timingEvent.param2 = event.param2;
            timingEvent.time = event.time + timingEvent.param2;
        }

        outEvents->push_back(event);
    }

    return outEvents;
}

std::unique_ptr<std::vector<Event>> SplitTime(std::vector<Event>& inEvents)
{
    std::unique_ptr<std::vector<Event>> outEvents(new std::vector<Event>());

    std::int32_t time = 0;

    for (const Event& event : inEvents)
    {
        std::int32_t diff = event.time - time;

        if (diff > 96)
        {
            int wholeNoteCount = (diff - 1) / 96;
            diff -= 96 * wholeNoteCount;

            for (int i = 0; i < wholeNoteCount; i++)
            {
                time += 96;
                Event timeSplitEvent = {};
                timeSplitEvent.time = time;
                timeSplitEvent.type = EventType::TimeSplit;
                outEvents->push_back(timeSplitEvent);
            }
        }

        std::int32_t lutValue = g_noteDurationLUT[diff];

        if (lutValue != diff)
        {
            Event timeSplitEvent = {};
            timeSplitEvent.time = time + lutValue;
            timeSplitEvent.type = EventType::TimeSplit;
            outEvents->push_back(timeSplitEvent);
        }

        time = event.time;

        outEvents->push_back(event);
    }

    return outEvents;
}

std::unique_ptr<std::vector<Event>> CreateTies(std::vector<Event>& inEvents)
{
    std::unique_ptr<std::vector<Event>> outEvents(new std::vector<Event>());

    for (const Event& event : inEvents)
    {
        if (event.type == EventType::Note && event.param2 > 96)
        {
            Event tieEvent = event;
            tieEvent.param2 = -1;
            outEvents->push_back(tieEvent);

            Event eotEvent = {};
            eotEvent.time = event.time + event.param2;
            eotEvent.type = EventType::EndOfTie;
            eotEvent.note = event.note;
            outEvents->push_back(eotEvent);
        }
        else
        {
            outEvents->push_back(event);
        }
    }

    return outEvents;
}

void CalculateWaits(std::vector<Event>& events)
{
    g_initialWait = events[0].time;
    int wholeNoteCount = 0;

    for (unsigned i = 0; i < events.size() && events[i].type != EventType::EndOfTrack; i++)
    {
        events[i].time = events[i + 1].time - events[i].time;

        if (events[i].type == EventType::TimeSignature)
        {
            events[i].type = EventType::WholeNoteMark;
            events[i].param2 = wholeNoteCount++;
        }
    }
}

int CalculateCompressionScore(std::vector<Event>& events, int index)
{
    int score = 0;
    std::uint8_t lastParam1 = events[index].param1;
    std::uint8_t lastVelocity = 0x80u;
    EventType lastType = events[index].type;
    std::int32_t lastDuration = 0x80000000;
    std::uint8_t lastNote = 0x40u;

    if (events[index].time > 0)
        score++;

    for (int i = index + 1; !IsPatternBoundary(events[i].type); i++)
    {
        if (events[i].type == EventType::Note)
        {
            int val = 0;

            if (events[i].note != lastNote)
            {
                val++;
                lastNote = events[i].note;
            }

            if (events[i].param1 != lastVelocity)
            {
                val++;
                lastVelocity = events[i].param1;
            }

            std::int32_t duration = events[i].param2;

            if (g_noteDurationLUT[duration] != lastDuration)
            {
                val++;
                lastDuration = g_noteDurationLUT[duration];
            }

            if (duration != lastDuration)
                val++;

            if (val == 0)
                val = 1;

            score += val;
        }
        else
        {
            lastDuration = 0x80000000;

            if (events[i].type == lastType)
            {
                if ((lastType != EventType::Controller && (int)lastType != 0x25 && lastType != EventType::EndOfTie) || events[i].param1 == lastParam1)
                {
                    score++;
                }
                else
                {
                    score += 2;
                }
            }
            else
            {
                score += 2;
            }
        }

        lastParam1 = events[i].param1;
        lastType = events[i].type;

        if (events[i].time)
            score++;
    }

    return score;
}

bool IsCompressionMatch(std::vector<Event>& events, int index1, int index2)
{
    if (events[index1].type != events[index2].type ||
        events[index1].note != events[index2].note ||
        events[index1].param1 != events[index2].param1 ||
        events[index1].time != events[index2].time)
        return false;

    index1++;
    index2++;

    do
    {
        if (events[index1] != events[index2])
            return false;

        index1++;
        index2++;
    } while (!IsPatternBoundary(events[index1].type));

    return IsPatternBoundary(events[index2].type);
}

void CompressWholeNote(std::vector<Event>& events, int index)
{
    for (int j = index + 1; events[j].type != EventType::EndOfTrack; j++)
    {
        while (events[j].type != EventType::WholeNoteMark)
        {
            j++;

            if (events[j].type == EventType::EndOfTrack)
                return;
        }

        if (IsCompressionMatch(events, index, j))
        {
            events[j].type = EventType::Pattern;
            events[j].param2 = events[index].param2 & 0x7FFFFFFF;
            events[index].param2 |= 0x80000000;
        }
    }
}

void Compress(std::vector<Event>& events)
{
    for (int i = 0; events[i].type != EventType::EndOfTrack; i++)
    {
        while (events[i].type != EventType::WholeNoteMark)
        {
            i++;

            if (events[i].type == EventType::EndOfTrack)
                return;
        }

        if (CalculateCompressionScore(events, i) >= 6)
        {
            CompressWholeNote(events, i);
        }
    }
}

void ReadMidiTracks()
{
    long trackHeaderStart = 14;

    ReadMidiTrackHeader(trackHeaderStart);
    ReadSeqEvents();

    g_agbTrack = 1;

    for (int midiTrack = 0; midiTrack < g_midiTrackCount; midiTrack++)
    {
        trackHeaderStart += ReadMidiTrackHeader(trackHeaderStart);

        for (g_midiChan = 0; g_midiChan < 16; g_midiChan++)
        {
            ReadTrackEvents();

            if (s_minNote != 0xFF)
            {
#ifdef DEBUG
                printf("Track%d = Midi-Ch.%d\n", g_agbTrack, g_midiChan + 1);
#endif

                std::unique_ptr<std::vector<Event>> events(MergeEvents());

                // We don't need TEMPO in anything but track 1.
                if (g_agbTrack == 1)
                {
                    auto it = std::remove_if(s_seqEvents.begin(), s_seqEvents.end(), [](const Event& event) { return event.type == EventType::Tempo; });
                    s_seqEvents.erase(it, s_seqEvents.end());
                }

                ConvertTimes(*events);
                events = InsertTimingEvents(*events);
                events = CreateTies(*events);
                std::stable_sort(events->begin(), events->end(), EventCompare);
                events = SplitTime(*events);
                CalculateWaits(*events);

                if (g_compressionEnabled)
                    Compress(*events);

                PrintAgbTrack(*events);

                g_agbTrack++;
            }
        }
    }
}

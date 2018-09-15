# `.blip` File Format

The `.blip` file format is a plain-text format intended to create chiptunes music. It consists of commands which are compiled and then executed by an interpreter. For example, the code for generating a tone would look like this:

```blip
a:c4
s:4
r
```

Where `a:c4` sets (attacks) the note C on octave 4, `s:4` advances the time by 4 steps and `r` releases the note. One step has a specific time unit which can be changed.

The command name and its arguments are separated by colons `:`. To write more compact code, commands can be seperated by semicolons and written on the same line:

```blip
a:c4;s:4;r
```

Terminating semicolons are not required. Either line breaks or semicolons terminate a command. Whitespace is allowed around separator characters like `:` or `;`, so this is valid too:

```blip
a:c4; s : 4; r
```

Comments are introduced with `%` and tell the parser to ignore content until the end of the line:

```blip
% this is a comment

a:c4;s:4;r % a comment at the end
```

## Table of Contents

- [Attack Command `a`](#attack-command-a)
    - [Arpeggio Sequences](#arpeggio-sequences)
    - [Fine-tuning Notes](#fine-tuning-notes)
- [Step Command `s`](#step-command-s)
- [Step Ticks Command `st`](#step-ticks-command-st)
    - [Calculating BPM](#calculating-bpm)
- [Waveform Command `w`](#waveform-command-w)
    - [Square Wave](#square-wave)
    - [Triangle Wave](#triangle-wave)
    - [Sawtooth Wave](#sawtooth-wave)
    - [Noise](#noise)
    - [Sine Wave](#sine-wave)
- [Duty Cycle Command `dc`](#duty-cycle-command-dc)
- [Release Command `r`](#release-command-r)
- [Mute Command `m`](#mute-command-m)
- [Volume Command `v`](#volume-command-v)
- [Panning Command `p`](#panning-command-p)
- [Pitch Command `pt`](#pitch-command-pt)
- [Effect Command `e`](#effect-command-e)
    - [Step Fractions](#step-fractions)
    - [Volume Slide Effect `e:vs`](#volume-slide-effect-evs)
    - [Panning Slide Effect `e:ps`](#panning-slide-effect-eps)
    - [Portamento Effect `e:pr`](#portamento-effect-epr)
    - [Tremolo Effect `e:tr`](#tremolo-effect-etr)
        - [Sliding Tremolo Values](#sliding-tremolo-values)
    - [Vibrato Effect `e:vb`](#vibrato-effect-evb)
        - [Sliding Vibrato Values](#sliding-vibrato-values)
- [Attack Ticks Command `at`](#attack-ticks-command-at)
- [Release Ticks Command `rt`](#release-ticks-command-rt)
- [Mute Ticks Command `mt`](#mute-ticks-command-mt)
- [Phase Wrap Command `pw`](#phase-wrap-command-pw)
- [Master Volume Command `vm`](#master-volume-command-vm)
- [Tick Rate Command `tr`](#tick-rate-command-tr)
- [Command Groups](#command-groups)
    - [Calling Group from other Tracks](#calling-group-from-other-tracks)
- [Tracks](#tracks)
    - [Global Track](#global-track)
- [Custom Waveforms](#custom-waveforms)
- [Instrument Command `i`](#instrument-command-i)
- [Instruments](#instruments)
    - [Sequence Phases](#sequence-phases)
    - [Interpolated Sequences](#interpolated-sequences)
    - [Pitch Sequence](#pitch-sequence)
    - [Volume Sequence](#volume-sequence)
    - [Panning Sequence](#panning-sequence)
    - [Duty Cycle Sequence](#duty-cycle-sequence)
- [Sample Command `d`](#sample-command-d)
- [Sample Repeat Command `dr`](#sample-repeat-command-dr)
- [Sample Sustain Range Command `ds`](#sample-sustain-range-command-ds)
- [Sample Range Command `dn`](#sample-range-command-dn)
    - [Reversing Samples](#reversing-samples)
- [Samples](#samples)

## Attack Command `a`

```blip
a:<note>
```

This command sets a single note or an arpeggio sequence.

```blip
% set note F# on octave 2
a:f#2
```

Notes range from `c0` to `c8`, where `c8` is the last playable note. (`c#8` does not exist.)

| Note Name | Octave Range | Example |
|---|---|---|
| c | 0-8 | `c5` |
| c# | 0-7 | `c#2` |
| d | 0-7 | `d0` |
| d# | 0-7 | `d#4` |
| e | 0-7 | `e1` |
| f# | 0-7 | `f#7` |
| g | 0-7 | `g2` |
| g# | 0-7 | `g#4` |
| a | 0-7 | `a5` |
| a# | 0-7 | `a#1` |
| b or h | 0-7 | `b3`, `h3` |

### Arpeggio Sequences

```blip
a:<note1>:<note2>...
```

Arpeggio sequences are multiple notes played fast and repeatedly after each other. Each note is played for 4 *ticks* by default. As a single track can only play on note at the time, this can be used to "simulate" a chord. For example, a minor F chord would look like this:

```blip
a:f3:g#3:c4
```

The maximum number of arpeggio notes is limited to 8:

```blip
a:c3:d3:e3:f3:g3:b3:c4:d4
```

The length of a single arpeggio note can be set with the arpeggio speed command `as:<ticks>`, which sets the number of *ticks* per note:

```blip
% set 8 ticks per arpeggio note
as:8

% play major C chord
a:c4:d#4:g4
% wait for 8 step
s:8
% release note
r

% reset arpeggio speed to default
as:0
```

### Fine-tuning Notes

It is possible to tune notes with *cents*. 100 cents is equals to one half-note. Value can be positive or negative. The syntax look like this: `<note>[+-]<cents>`.

```blip
% play note g5 + 45 cents (+0.45 half-notes)
a:g5+45

% play note a#2 - 30 cents (-0.30 half-notes)
a:g#2-30

% play note d4 + 1200 cents (+12 half-notes = 1 octave)
a:d4+1200   % equal to d5
```
## Step Command `s`

```blip
s:<steps>
```

The step command `s` advances the time by a certain number of steps. Each step consists of a certain amount of [ticks](step-ticks-command-st). One step has a duration of 0.1s by default.

```blip
% set note
a:d2

% wait for 5 steps
s:5

% set another note
a:f2

% wait for 3 steps
s:3

% release note
r
```

## Step Ticks Command `st`

```blip
st:<ticks>
```

This commands defines the number of ticks per step. The default is 24. One tick has a duration of 1/240th of a second by default and can be changed with the [tick rate command](tick-rate-command-tr).

```blip
% set step ticks
st:21

% set note
a:d2

% wait for 5 steps (5 steps * 21 ticks = 105 ticks)
s:5

% set another note
a:f2

% wait for 3 steps (3 steps * 21 ticks = 63 ticks)
s:3

% release note
r
```

### Calculating BPM

The follwing formula can be used to calculate the number of beats per minute from the number of step ticks:

```
bpm = tr * 60 / st / 4
```

Where `tr` is the [tick rate](#tick-rate-command-tr) (default is 240) and `st` is the number of ticks per step defined with the [step ticks command](#step-ticks-command-st). The number `4` indicated the number of steps per bar. If another number of steps is needed, the number `4` can be replaced with such.

## Waveform Command `w`

```blip
w:<wave>
```

The waveform command changes the currently used waveform:

```blip
% set sawtooth waveform
w:sawtooth

% play notes
a:c2;s:4;r;s:4
a:c1;s:4;r;s:4
```

The followng table lists the default waveforms:

| Waveform | Names | Master Volume |
|---|---|---|
| Square | `square`, `sqr` | 0.15 (38) |
| Triangle | `triangle`, `tri` | 0.3 (76) |
| Sawtooth | `sawtooth`, `was` | 0.15 (38)  |
| Noise | `noise`, `noi` | 0.15 (38) |
| Sine | `sine`, `sin` | 0.3 (76) |
| [Samples](#samples) | `sample`, `smp` | 0.3 (76) |

It is also possible to define [custom waveforms](#custom-waveforms). Use the command `w:<name>` to set a custom waveform:

```blip
% use waveform 'aah'
w:aah

% play note
a:c3;s:4;r
```

Waveforms are made of individual amplitude changes. Each waveform has a specific number of these *phases*.

*Changing the waveform also changes the current [master volume](#master-volume-command-vm).*

### Square Wave

```blip
square, sqr
```

The square wave has 16 phases. The length of the high period (duty cycle) and can be changed with the [duty cycle command](#duty-cycle-command-dc).

```
max | # # # # 
    | # # # # 
    | # # # #
    | # # # #
  0 | +-+-+-+-#-#-#-#-#-#-#-#-#-#-#-#
```

### Triangle Wave

```blip
triangle, tri
```

The triangle wave has 32 phases. It is the only waveform, which is not affected by the [volume command](#volume-command-v) or the [instrument's volume envelope](#volume-sequence).

```
max |                 #
    |               # # #
    |             # # # # #
    |           # # # # # # #
    |         # # # # # # # # #
    |       # # # # # # # # # # #
    |     # # # # # # # # # # # # #
    |   # # # # # # # # # # # # # # #
  0 | #-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                   # # # # # # # # # # # # # # #
    |                                     # # # # # # # # # # # # #
    |                                       # # # # # # # # # # #
    |                                         # # # # # # # # #
    |                                           # # # # # # #
    |                                             # # # # #
    |                                               # # #
min |                                                 #
```

### Sawtooth Wave

```blip
sawtooth, saw
```

The sawtooth wave has 7 phases.

```
max | #
    | # #
    | # # #
    | # # # #
    | # # # # #
    | # # # # # #
  0 | +-+-+-+-+-+-#
```

### Noise

```blip
noise, noi
```

Noise is actually a square wave whose amplitudes are randomly high or low. Its are generated with a 16 bit linear recurrences random number generator. This means, that the noise pattern repeats after 2^16 phases.

```
max | #   # #   #     # # #   #   #
    | #   # #   #     # # #   #   #
    | #   # #   #     # # #   #   #
    | #   # #   #     # # #   #   #
  0 | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - -
```

### Sine Wave

```blip
sine, sin
```

The sine wave has 32 phases. The following amplitudes are an approximation.

```
max |               # # #
    |           # # # # # # #
    |         # # # # # # # # #
    |         # # # # # # # # #
    |       # # # # # # # # # # #
    |     # # # # # # # # # # # # #
    |     # # # # # # # # # # # # # #
    |   # # # # # # # # # # # # # # #
  0 | #-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                   # # # # # # # # # # # # # # #
    |                                   # # # # # # # # # # # # # #
    |                                     # # # # # # # # # # # # #
    |                                       # # # # # # # # # # #
    |                                         # # # # # # # # #
    |                                         # # # # # # # # #
    |                                           # # # # # # #
min |                                               # # #                
```

## Duty Cycle Command `dc`

```blip
dc:<number>
```

The ducy cycle command sets the length of the [square wave](#square-wave)'s high period. The value ranges from 1 to 15. The default value is 4:

```blip
% set duty cycle to 8 (50%)
dc:8
% play note
a:c4;s:4;r;s:4

% set duty cycle to 4 (25%)
dc:4
% play note
a:c4;s:4;r;s:4

% set duty cycle to 2 (12.5%)
dc:2
% play note
a:c4;s:4;r;s:4
```

## Release Command `r`

```blip
r
```

This command releases the currently set note. If an [instrument](#instruments) is set, the release phase of each sequence is played.

```blip
% set note c4
a:c4
% wait for 4 steps
s:4

% release note
r
```

## Mute Command `m`

```blip
m
```

This command behaves like `r` if no instrument is set. If an [instrument](#instruments) is set, the instrument's release sequence is not played.

```blip
% set note c4
a:c4
% wait for 4 steps
s:4

% mute note
m
```

## Volume Command `v`

```blip
v:<volume>
```

The volume command sets the current volume to a value between 0 and 255, where 255 is the default:

```blip
% set volume to 63
v:192

% play note
a:e3
% wait for 4 steps
s:4

% set volume to 63
v:127

% wait for 4 steps
s:4
% release note
r
```

## Panning Command `p`

```blip
p:<panning>
```

The panning command sets the stereo direction to a value between -255 and 255 which corresponds to left and right, respectively. The default value is 0.

```blip
% set panning to -127
p:-127

% play note
a:e3
% wait for 4 steps
s:4

% set panning to +127
p:127

% wait for 4 steps
s:4
% release note
r
```

## Pitch Command `pt`

```blip
pt:<cents>
```

The pitch command add a certain number of [cents](#fine-tuning-notes) to all notes played after the command.

```blip
% raise each note by 7 half-tones
pt:700

% set note c3 which will play as g3
a:c3

% lower each note by 3 half-tones
pt:-300

% set note c3 which will play as a2
a:c3

% reset pitch
pt:0
```

## Effect Command `e`

```blip
e:<effect>...
```

The effect command enables and changes various effects. Time units used for the arguments can be expressed either with *ticks* or *step fractions*.

### Step Fractions

This time unit is defined as fraction of a *step*. The notation may by more handy to use than *ticks*. It has the syntax: `<nominator>/<denoninator>`. For example:

| Step fraction | Description |
|---|---|---|
| `1/2` | 1/2 of a step. If `st` is set to 24 the value equals to 12 ticks. |
| `3/2` | 2/3 of a step. If `st` is set to 24 the value equals to 36 ticks. |
| `1/1` | 1/1 of a step which equals to 1 step. |
| `8/1` | 8/1 of a step which equals to 8 steps. |

- [Volume Slide Effect](#volume-slide-effect)
- [Panning Slide Effect](#panning-slide-effect)
- [Portamento Effect](#portamento-slide-effect)
- [Tremolo Effect](#tremolo-slide-effect)
- [Vibrato Effect](#vibrato-slide-effect)

### Volume Slide Effect `e:vs`

```
e:vs:<ticks>
```

This effect slides the current volume to the new value set with `v` within a certain amount of time. For example:

```blip
% set volume to 0
v:0
% set note
a:c4

% enable volume slide effect using 8 steps
e:vs:8/1

% set new volume
v:255
% advance time by 8 steps
s:8
% release note
r

% disable the effect
e:vs

```

If the effect is disable before the volume has reached its new value, the volume jumps to the target value.

### Panning Slide Effect `e:ps`

```
e:ps:<ticks>
```

This effect slides the current panning value to the new value set with `p` within a certain amount of time. For example:

```blip
% set panning to -255
p:-255
% set note
a:c4

% enable panning slide effect using 8 steps
e:ps:8/1

% set new panning
p:255
% advance time by 8 steps
s:8
% release note
r

% disable the effect
e:ps

```

If the effect is disable before the panning value has reached its new value, the panning value jumps to the target value.

### Portamento Effect `e:pr`

```blip
e:pr:<ticks>
```

This effect slides the current note to the new note set with `a`. If currently no note is set, it has no effect. If the newly set note is an arpeggio sequence, it slides to the first note in the sequence. For example:

```blip
% slide note in 4 steps
e:pr:4/1

% set note
a:c4
% slide to note
a:c5
% wait 8 steps
s:8
% release note
r

% set note
a:c4
% wait 4 steps
s:4
% slide to note
a:c3
% wait 4 steps
s:4
% release note
r
```

If the effect is disable before the note has reached its new value, the note jumps to the target value.

### Tremolo Effect `e:tr`

```
e:tr:<ticks>:<amount>
```

This effect periodically decreases the volume by a certain amount.

A tremolo cycle consists of 2 phases: decrease and increase. Each phase has the duration given by `<ticks>`. The volume in the decrease phase is reduced by the value given with `<amount>`. For example:

```blip
% reduce volume by 127 every 1/2 step
e:tr:1/2:127

% play note for 8 steps
a:c4;s:8;r

% disable the effect
e:tr
```

#### Sliding Tremolo Values

```
e:tr:<ticks>:<amount>:<slide ticks>
```

It is possible to slide the effect values itself to new values by providing a third parameter. For example:

```blip
% change volume by 127 every 1/2 step
e:tr:1/2:127

% play note for 8 steps
a:c4
% wait 8 steps
s:8

% slide to new values within 8 steps
e:tr:1/8:192:8/1

% wait 16 steps
s:16

% disable the effect
e:tr
```

### Vibrato Effect `e:vb`

```
e:vb:<ticks>:<amount>
```

This effect periodically decreases and increases the pitch by a certain amount. It is independent from the pitch command `pt`.

A vibrato cycle consists of 4 phases: increase by amount, decrease to 0, decrease by amount, increase to 0 again. Each phase has the duration given by `<ticks>`. The pitch in each phase is changed by the value given with `<amount>`, which has the unit [cents](#fine-tuning-notes). For example:

```blip
% change pitch every 1/2 step by 2 half-tones
e:vb:1/2:200

% play note for 8 steps
a:c4;s:8;r

% disable the effect
e:tr
```

#### Sliding Vibrato Values

```
e:vb:<ticks>:<amount>:<slide ticks>
```

It is possible to slide the effect values itself to new values by providing a third parameter. For example:

```blip
% change pitch every 1/2 step by 2 half-tones
e:vb:1/2:200

% play note for 8 steps
a:c4
% wait 8 steps
s:8

% slide to new values within 8 steps
e:vb:1/8:50:8/1

% wait 16 steps
s:16

% disable the effect
e:tr
```

## Attack Ticks Command `at`

```blip
at:<ticks>
```

This command delays the next attack command `a` by a given number of *ticks* or [step fraction](#step-fractions) relative to the current time position. This can be used to add *swing*.

```blip
        % play note
        a:c4;s:1;r;s:1
        % delay note by 1/3 step
at:1/3; a:c4;s:1;r;s:1

        % play note
        a:d#4;s:1;r;s:1
        % delay note by 1/3 step
at:1/3; a:f4;s:1;r;s:1

        % play note
        a:g4;s:3;r;s:1
```

## Release Ticks Command `rt`

```blip
rt:<ticks>
```

This command adds a delayed release command `r` relative to the current time position in number of *ticks* or [step fraction](#step-fractions). This can be used to play short notes.

```blip
% play note for 1/3 step
rt:1/3; a:c4;s:4

% play note for 2/3 step
rt:2/3; a:d#4;s:4

% play note for 6/3 steps (2 steps)
rt:6/3; a:a4;s:4
```

A combination of `at` and `rt` can be used to shift a note:

```blip
% begin note at 1/3 step and release it at 2/3 step
% so the note has a length of 1/3 step
at:1/3; rt:2/3; a:c4;s:1
```

## Mute Ticks Command `mt`

```blip
mt:<ticks>
```

This command adds a delayed mute command `m` relative to the current time position in number of *ticks* or [step fraction](#step-fractions). This command behaves like `rt` if no instrument is set. If an [instrument](#instruments) is set, then the intrument's release sequence is not played.

## Phase Wrap Command `pw`

```blip
pw:<count>
```

This commands sets the number of phases after which a [waveform](#waveform-command-w) should reset. This can be used as an interesting effect, for example, using with [noise](noise):

```blip
% use noise
w:noise

% play note
a:c4;s:8;r

% reset waveform after 64 phases
pw:64

% play note
a:c4;s:8;r

% reset
pw:0
```

## Master Volume Command `vm`

```blip
vm:<volume>
```

The master volume sets the volume of a [track](#tracks) and has a value between 0 and 255. The master volume `vm` and note volume `v` are multiplied to calculate the final volume.

When changing the master volume for, one has to consider that the amplitudes of multiple tracks are added together and may result in an overflowing amplitude.

Every default [waveform](#waveform-command-w) has its specific master volume which will be reset when a new waveform is set.

## Tick Rate Command `tr`

```blip
tr:<duration>
```

This commands sets the duration of one *tick*. It is defined as a fraction of a second. The default is 240.

```blip
% define tick as 1/480th of a second
tr:480

% define tick as 7/960th of a second
tr:7/960
```

## Command Groups

```blip
[grp:<number> ...]
```

Commands or patterns which are used several times can be grouped with command groups. Groups are defined with `grp:<number>` and enclose their commands in square brackets `[...]`. Where `<number>` is a number between 0 and 255 that can be defined freely, but only used once per [track](tracks).

Groups can be *called* like functions with `g:<group>` and can call other groups as well.

```blip
% define group 0
[grp:0
    a:c4;s:1;r;s:1
    a:a#3;s:1;r;s:1
    a:g3;s:2;r;s:2
    a:f3;s:3;r;s:1

    % call group 1
    g:1
]

% define group 1
[grp:1
    a:g3;s:1;r;s:1
    a:d#4;s:1;r;s:1
    a:f3;s:2;r;s:2
    a:g3;s:3;r;s:1
]

% call group 0
g:0

% set pitch to raise notes by 5 half-tones
pt:500
% call group 0
g:0
```

If the `<number>` of the group definition is omitted, the next free slot is used.

> A group could theoretically call itself. Although the call chain would end after 16 call as the stack is limit to 16 nested calls.

### Calling Group from other Tracks

Calling a globally defined group `3`:

```blip
g:3g
```

Calling a group `3` defined in track `4`:

```blip
g:3t4
```

## Tracks

```blip
[track:<wave> ...]
```

Tracks allow to execute multiple command sequences (and therefore notes) in parallel. They are independent from each other and commands from one track do not influence other tracks, except for the global commands `st` ([step ticks](#step-ticks-command-st)) and `tr` ([tick rate](#tick-rate-command-tr)).

Tracks are defined with `track:<wave>` and enclose commands (including [groups](#command-groups)) in square brackets `[...]`. Where `<wave>` defines the initial [waveform](#waveform-command-w).

```blip
% define track with square wave
[track:square
    [grp:0
        a:c4;s:1;r;s:1
        a:a#3;s:1;r;s:1
        a:g3;s:2;r;s:2
        a:f3;s:3;r;s:1
    ]

    % set effect
    e:vb:1/2:20

    % play group 0
    g:0

    a:g3;s:1;r;s:1
    a:d#4;s:1;r;s:1
    a:f3;s:2;r;s:2
    a:g3;s:3;r;s:1
]

% define track with triangle wave
[track:triangle
    % set pitch
    pt:-2400

    % play group 0 from track 0 (the above)
    g:0t0
]
```

Tracks can be assigned to slots (like groups) as well, using the syntax `[track:<wave>:<number> ...]`. This is usually not needed, although it can be usefull when calling [groups of different tracks](#calling-group-from-other-tracks).

### Global Track

The global track is a separate track which contains all commands, that are not inside a `[track ...]` declaration.

## Custom Waveforms

```blip
[wave:<name> ...]

w:<name>
```

Custom waveforms are defined with `wave:<name>` and enclose the definition in square brackets `[...]`. Where `<name>` is an arbitary unique name, but not equal to any [default waveform](#waveform-command-w) name.

The amplitude phases are defined inside the square brackets using the command `s`. The values range from 255 to -255. The number of phases has a minimum of 2 and a maximum of 64.

```blip
% define waveform named 'aah'
[wave:aah
    % define amplitude phases
    s:-255:-163:-154:-100:45:127:9:-163:-163:-27:63:72:63:9:-100:
        -154:-127:-91:-91:-91:-91:-127:-154:-100:45:127:9:-163:-163:9:127:45
]
```

## Instrument Command `i`

```blip
i:<name>
```

This commands sets a defined [instrument](#instruments):

```blip
% use instrument 'lead1'
i:lead1

% play note
a:c3;s:4;r

% disable an instrument
i
```

## Instruments

```blip
[instr:<name> ...]
```

Instruments define envelope sequences for certain values like volume or pitch. They are defined with `instr:<name>` and enclose the definition in square brackets `[...]`. Where `<name>` is an arbitary unique name.

Instruments can have multiple sequences, which are lists of values changing the value of their corresponding type. Sequences have 3 phases which consist of an arbitary number of values. Each sequence phase (value) is played for 4 *ticks*.

The following sequence types are defined:

| Type | Sequence Name | [Interpolated Sequence Name](#interpolated-sequences) |
|---|---|---|
| [Pitch](#pitch-sequence) | `a` | `anv` |
| [Volume](#volume-sequence) | `v` | `vnv` |
| [Panning](#panning-sequence) | `p` | `pnv` |
| [Duty Cycle](#duty-cycle-sequence) | `dc` | `dcnv` |

### Sequence Phases

```blip
<name>:{attack}:<:{sustain}:>:{release}
```

- **Attack phase**: the first part before the `<` which is played after a note is set with `a`.
- **Sustain phase**: the part between the enclosing `<` and `>` which is played repeatedly as long as the note is set.
- **Release phase**: the part after the `>` which is played after the note is release with `r`.

A pitch sequence definition may look like this:

```blip
% define pitch sequence with all phases
% { attack }{ sustain }{ release }
a:1200:700:<:0:-1200:>:-300:-700:-1200
```

- `a` the envelope type (pitch)
- `1200` raise pitch by 1200 *cents* (1 octave)
- `700` raise pitch by 700 *cents* (7 half-tones)
- `<` begin of sustain phase
- `0` keep pitch a 0
- `-1200` lower pitch by 1200 *cents* (1 octave)
- `>` end of sustain phase
- `-300` lowers pitch by 300 *cents* (3 half-tones)
- `-700` lowers pitch by 500 *cents* (7 half-tones)
- `-1200` lowers pitch by 1200 *cents* (1 octave)

An instrument definition using a pitch and volume sequence may look like this:

```blip
% define instrument 'lead1'
[instr:lead1
    % define pitch sequence with all phases
    % { attack }{ sustain }{ release }
    a:1200:700:<:0:-1200:>:0:0:0:1200

    % define volume sequence with only an attack phase
    % {        attack         }
    v:255:192:178:127:64:127:64
]
```

The attack phase can be omitted, so that the sequence consists only of a sustain and release phase:

```blip
% define pitch sequence without attack phase
% { sustain }{ release }
a:<:0:-1200:>:0:0:0:1200
```

The release phase can be omitted, so that the sequence consists only of an attack and sustain phase. In such a sequence, the last value which was active in the sustain phase is hold until all other sequences have completed their release phase.

```blip
% define pitch sequence without release phase
% { attack }{ sustain }
a:1200:1200:<:0:-1200:>
```

When ommiting the sustain phase the sequence consist only of a single attack phase, which hold its last value until the note is released:

```blip
% define pitch sequence with only an attack phase
% if the sequence reaches 700, the value is hold until the note is released
% {  attack   }
a:0:300:500:700
```

### Interpolated Sequences

This type of sequence interpolates the phase values and creates smoother changes. It uses a pair of values per phase, where the first element defines the duration in *ticks* and the second one is the value itself. For example:

```blip
% define interpolated pitch sequence
%  { attack } {   sustain    } { release }
anv:0:0:      <:48:400:48:0:>: 96:-2400
```

The spacing is not necessary, it should help to visualize the different parts.

- `anv` the envelope type (pitch)
- `0:0` immediately (in 0 ticks) set the pitch value to 0
- `<` begin of sustain phase
- `48:400` raise pitch within 48 *ticks* by 400 *cents* (4 half-notes)
- `48:0` lower pitch to 0 within 48 *ticks*
- `>` end of sustain phase
- `96:-2400` lower pitch by 2400 *cents* (2 octaves) within 96 *ticks*

A working example:

```blip
% define instrument 'alarm'
[instr:alarm
    % define "alarm"-like pitch sequence
    anv:0:0:<:48:400:48:0:>:96:-2400
]

% set instrument 'alarm'
i:alarm

% play note
a:c4;s:32;r
% wait for release phase
s:16
```

> The sustain phase's total duration must be greater than 0, or an error will be generated.

### Pitch Sequence

```blip
a:{attack}:<:{sustain}:>:{release}
anv:{attack}:<:{sustain}:>:{release}
```

This sequence changes the current pitch by a certain amount. The values are defined in *cents*.

```blip
[instr:lead
    % define pitch sequence
    a:1200:<:0:300:700:>:-1200:-1200
]
```

```blip
[instr:lead
    % define pitch envelope
    anv:24:1200:<:48:0:48:300:48:700:>:96:-1200
]
```

> The 2 variants overwrite each other.

### Volume Sequence

```blip
v:{attack}:<:{sustain}:>:{release}
vnv:{attack}:<:{sustain}:>:{release}
adsr:<attack>:<decay>:<sustain>:<release>
```

This sequence changes the current volume. The values are multiplied with the current volume (divided by 255) and can range from 0 to 255.

```blip
[instr:lead
    % define volume sequence
    v:64:127:192:<:255:127:>:127:64:0:0:0:64:0:64:0
]
```

```blip
[instr:lead
    % define volume envelope
    vnv:12:192:<:48:255:48:127:>:48:0:72:0:12:64:12:64:12:0
]
```

There is a convenient notation of the ADSR envelope using the syntax `adsr:<attack ticks>:<decay ticks>:<sustain volume>:<release ticks>`:

```blip
[instr:lead
    % define ADSR envelope
    adsr:12:8:192:96

    % this corresponds to this envelope
    vnv:12:255:8:192:<:1:192:>:96:0
]
```

> The 3 variants overwrite each other.

### Panning Sequence

```blip
p:{attack}:<:{sustain}:>:{release}
pnv:{attack}:<:{sustain}:>:{release}
```

This sequence changes the current panning value. The values are added to the current padding value and can range from -255 to 255.

```blip
[instr:lead
    % define panning sequence
    p:0:<:-32:-64:-32:0:32:64:32:0:>:0:0:16:-16:0
]
```

```blip
[instr:lead
    % define panning envelope
    pnv:0:0:<:24:-64:24:64:>:96:-255
]
```

> The 2 variants overwrite each other.

### Duty Cycle Sequence

```blip
dc:{attack}:<:{sustain}:>:{release}
dcnv:{attack}:<:{sustain}:>:{release}
```

This sequence overrides the current duty cycle value and is only appplied when the current waveform is a square wave. Values with 0 do not change the current value.

```blip
[instr:lead
    % define duty cycle sequence
    dc:1:2:4:6:<:8:5:3:2:1:>:0:0:0:1
]
```

```blip
[instr:lead
    % define panning envelope
    dcnv:0:1:<:48:1:48:8:>:96:1
]
```

> The sequence and envelope variants overwrite each other.

## Sample Command `d`

```blip
d:<name>
```

This commands sets a defined [sample](#samples) and replaces the current waveform:

```blip
% use sample 'bass'
d:bass

% play note
a:c4;s:8;r
```

Samples can also be combined with instruments:

```blip
% use sample 'bass'
d:bass

% use instrument 'lead1'
i:lead1

% play note
a:c4;s:8;r
```

Samples can't be disabled, but set to a new sample or replaced with a waveform. 

## Sample Repeat Command `dr`

```blip
dr:<mode>
```

This commands sets the [sample](#samples)'s repeat mode.

| Value | Description |
|---|---|
| `no` | Don't repeat. The sample is only played once after a note is set (one-shot). |
| `rep` | Repeat sustain range. The [sample's sustain range](#sample-sustain-range-command-dn) is repeated as long as the note is set. Its release phase is played after the note is released with `r`. |
| `pal` | Repeat sustain range using palindrome. The [sample's sustain range](#sample-sustain-range-command-dn) is played back and forth as long as the note is set. Its release range is played after the note is released with `r`. |

> This command must placed **after** the sample was set with `d`.

## Sample Sustain Range Command `ds`

```blip
ds:<start>:<end>
```

This commmand sets the [sample](#samples)'s sustain range. The values are the absolute frame offsets from the start of the sample defined by `dn`. By default, the full sample range is defined as the sustain range.

```blip
% define sustain range
ds:28421:34585
```

This will set the sustain range to the frames from `28421` to `34585-1`. `34585` itself is not included in the range. So the range has a total length of `34585 - 28421 = 6164`.

The range can be reset by setting both values to 0:

```blip
% reset sustain range
ds:0:0
```

> This command must placed **after** the sample was set with `d`.

## Sample Range Command `dn`

```blip
dn:<start>:<end>
```

This commmand sets the [sample](#samples)'s playable range. The values are the absolute frame offsets from the start of the sample. By default, the full sample range is played.

```blip
% define sustain range
dn:2865:56795
```

This will set the range to the frames from `2865` to `56795-1`. `56795 ` itself is not included in the range. So the range has a total length of `56795 - 2865 = 53930`.

> This command must placed **after** the sample was set with `d`.

### Reversing Samples

Switching the two values will play the sample backwards including the sustain range:

```blip
% define reversed sustain range
dn:56795:2865
```

Negative values define frame offsets from the end. `-1` is the last sample. This will play the whole sample backwards:

```blip
% play whole sample backwards
dn:-1:0
```

## Samples

```blip
[sample:<name> ...]
```

Samples are audio snippets which can be loaded from `.wav` files. The number of channels has to be either 2 (stereo) or 1 (mono). Only the PCM format with either 8 or 16 bits is supported.

They are defined with `sample:<name>` and enclose the definition in square brackets `[...]`. Where `<name>` is an arbitary unique name.

The command `load:<type>:<file>` loads an audio file. Where `<type>` is `wav` and `<file>` is the file path. For security reasons, files can only be loaded from the same directory or a sub-directory. Files loading from a parent directory, for example, `../sample.wav` is not allowed.

```blip
% define sample 'bass'
[sample:bass
    % load wave from file 'bass.wav'
    load:wav:bass.wav
]

% enable sample 'bass'
d:bass

% play sample in its orignal pitch
a:c4;s:8;r

% play sample in f4
a:f4;s:8;r
```

The commands [`dr`](#sample-repeat-command-dr), [`ds`](#sample-sustain-range-command-dr) and [`dn`](#sample-range-command) can also declared directly inside the sample definition:

```blip
% define sample 'lead'
[sample:lead
    % load wave from file 'lead.wav'
    load:wav:lead.wav
    % set sample range
    dn:4125:78964
    % set sample sustain range
    ds:23567:32342
    % set sample repeat mode
    dr:pal
]
```

#include <sound.h>
#include <lib.h>


#define C2  65
#define Cs2 69
#define D2  73
#define Ds2 78
#define E2  82
#define F2  87
#define Fs2 93
#define G2  98
#define Gs2 104
#define A2  110
#define As2 117
#define B2  123


#define C3  131
#define Cs3 139
#define D3  147
#define Ds3 156
#define E3  165
#define F3  175
#define Fs3 185
#define G3  196
#define Gs3 208
#define A3  220
#define As3 233
#define B3  247


#define C4  262
#define Cs4 277
#define D4  294
#define Ds4 311
#define E4  330
#define F4  349
#define Fs4 370
#define G4  392
#define Gs4 415
#define A4  440
#define As4 466
#define B4  494


#define C5  523
#define Cs5 554
#define D5  587
#define Ds5 622
#define E5  659
#define F5  698
#define Fs5 740
#define G5  784
#define Gs5 831
#define A5  880
#define As5 932
#define B5  988


#define CORCHEA    2
#define NEGRA      4
#define BLANCA     8
#define SILENCIO   1

void play_note(uint32_t frequency, int duration) {
    speaker_play(frequency);
    sleep_ticks(duration);
    speaker_stop();
    sleep_ticks(1);
}

void mario_bros(void) {
    println("\n>>> Reproduciendo: Super Mario Bros <<<\n");


    play_note(E5, CORCHEA);
    play_note(E5, CORCHEA);
    sleep_ticks(CORCHEA);
    play_note(E5, CORCHEA);
    sleep_ticks(CORCHEA);
    play_note(C5, CORCHEA);
    play_note(E5, NEGRA);
    play_note(G5, BLANCA);
    sleep_ticks(NEGRA);

    play_note(G4, BLANCA);
    sleep_ticks(NEGRA);


    play_note(C5, NEGRA);
    play_note(G4, NEGRA);
    play_note(E4, BLANCA);
    play_note(A4, NEGRA);
    play_note(B4, NEGRA);
    play_note(As4, CORCHEA);
    play_note(A4, NEGRA);


    play_note(G4, CORCHEA);
    play_note(E5, CORCHEA);
    play_note(G5, CORCHEA);
    play_note(A5, NEGRA);
    play_note(F5, CORCHEA);
    play_note(G5, CORCHEA);
    sleep_ticks(CORCHEA);
    play_note(E5, CORCHEA);
    play_note(C5, CORCHEA);
    play_note(D5, CORCHEA);
    play_note(B4, BLANCA);
    sleep_ticks(CORCHEA);


    play_note(C5, CORCHEA);
    play_note(G4, CORCHEA);
    play_note(E4, CORCHEA);
    play_note(A4, CORCHEA);
    play_note(B4, NEGRA);
    play_note(As4, CORCHEA);
    play_note(A4, CORCHEA);
    play_note(G4, CORCHEA);
    play_note(E5, CORCHEA);
    play_note(G5, CORCHEA);
    play_note(A5, NEGRA);
    play_note(F5, CORCHEA);
    play_note(G5, CORCHEA);
    play_note(E5, CORCHEA);
    play_note(C5, CORCHEA);
    play_note(D5, CORCHEA);
    play_note(B4, BLANCA);

    println(">>> Cancion finalizada! <<<");
}


void play_death_sound(void) {
    speaker_play(330);
    sleep_ticks(3);
    speaker_play(220);
    sleep_ticks(5);
    speaker_stop();
}
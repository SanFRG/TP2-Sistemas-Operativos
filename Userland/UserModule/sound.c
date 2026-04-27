#include <sound.h>
#include <lib.h>

// Notas musicales en Hz - Octava 2
#define C2  65
#define Cs2 69  // C#
#define D2  73
#define Ds2 78  // D#
#define E2  82
#define F2  87
#define Fs2 93  // F#
#define G2  98
#define Gs2 104 // G#
#define A2  110
#define As2 117 // A#
#define B2  123

// Octava 3
#define C3  131
#define Cs3 139 // C#
#define D3  147
#define Ds3 156 // D#
#define E3  165
#define F3  175
#define Fs3 185 // F#
#define G3  196
#define Gs3 208 // G#
#define A3  220
#define As3 233 // A#
#define B3  247

// Octava 4
#define C4  262
#define Cs4 277 // C#
#define D4  294
#define Ds4 311 // D#
#define E4  330
#define F4  349
#define Fs4 370 // F#
#define G4  392
#define Gs4 415 // G#
#define A4  440
#define As4 466 // A#
#define B4  494

// Octava 5
#define C5  523
#define Cs5 554 // C#
#define D5  587
#define Ds5 622 // D#
#define E5  659
#define F5  698
#define Fs5 740 // F#
#define G5  784
#define Gs5 831 // G#
#define A5  880
#define As5 932 // A#
#define B5  988

// Duraciones (en ticks, asumiendo ~18 ticks/seg)
#define CORCHEA    2   // nota corta
#define NEGRA      4   // nota media
#define BLANCA     8  // nota larga
#define SILENCIO   1   // pausa corta

void play_note(uint32_t frequency, int duration) {
    speaker_play(frequency);
    sleep_ticks(duration);
    speaker_stop();
    sleep_ticks(1);  // Pequeña pausa entre notas
}

void mario_bros(void) {
    println("\n>>> Reproduciendo: Super Mario Bros <<<\n");
    
    // Parte 1 - Tema principal
    play_note(E5, CORCHEA);
    play_note(E5, CORCHEA);
    sleep_ticks(CORCHEA);  // pausa
    play_note(E5, CORCHEA);
    sleep_ticks(CORCHEA);  // pausa
    play_note(C5, CORCHEA);
    play_note(E5, NEGRA);
    play_note(G5, BLANCA);
    sleep_ticks(NEGRA);  // pausa
    
    play_note(G4, BLANCA);
    sleep_ticks(NEGRA);  // pausa
    
    // Parte 2 - Continuación
    play_note(C5, NEGRA);
    play_note(G4, NEGRA);
    play_note(E4, BLANCA);
    play_note(A4, NEGRA);
    play_note(B4, NEGRA);
    play_note(As4, CORCHEA);
    play_note(A4, NEGRA);
    
    // Parte 3 - Progresión melódica
    play_note(G4, CORCHEA);
    play_note(E5, CORCHEA);
    play_note(G5, CORCHEA);
    play_note(A5, NEGRA);
    play_note(F5, CORCHEA);
    play_note(G5, CORCHEA);
    sleep_ticks(CORCHEA);  // pausa
    play_note(E5, CORCHEA);
    play_note(C5, CORCHEA);
    play_note(D5, CORCHEA);
    play_note(B4, BLANCA);
    sleep_ticks(CORCHEA);  // pausa
    
    // Parte 4 - Segunda parte
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

// Reproducir sonido de muerte
void play_death_sound(void) {
    speaker_play(330);  // Mi
    sleep_ticks(3);
    speaker_play(220);  // La (octava baja)
    sleep_ticks(5);
    speaker_stop();
}
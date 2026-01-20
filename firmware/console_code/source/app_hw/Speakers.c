#include "Speakers.h"
#include "sound.h"

cyhal_dac_t dac_obj;
static uint16_t audio_buffer[AUDIO_BUFFER_SIZE];
extern int16_t speaker_buffer[256];
static volatile size_t buffer_index = 0;
static volatile bool buffer_ready = false;

EventGroupHandle_t wall_event = NULL;



cy_rslt_t speakers_init() {
    cy_rslt_t result;

    wall_event = xEventGroupCreate();
    if (wall_event == NULL) {
        return CY_RSLT_TYPE_ERROR;  // Failed to create event group
    }
    
    result = cyhal_dac_init(&dac_obj, SPEAKER_PIN);

    if (CY_RSLT_SUCCESS != result)
    {
        return result;
        
    }

    cyhal_dac_write(&dac_obj, 32768);

    speaker_startup(speaker_buffer, 256, 16800);
    speaker_mario_coin(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);

    // beep(speaker_buffer, 256, 230000, 680);
    // beep(speaker_buffer, 256, 400000, 980);
    // speaker_victory(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
    // speaker_mario(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
    // speaker_mario_victory(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
    // speaker_pokemon(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
    // wall_hit_note(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ, 880.0);
    

    return result;   
}

int speaker_write(const int16_t* data, size_t length) {

    if (buffer_ready)
    {
        return -1;
    }
    
    if (length > AUDIO_BUFFER_SIZE)
    {
        length = AUDIO_BUFFER_SIZE;
    }
    
    for (size_t i = 0; i < length; i++)
    {
       int32_t sample = (int32_t)data[i] + 32768;  

        if (sample < 0) sample = 0;
        if (sample > 65535) sample = 65535;

        audio_buffer[i] = (uint16_t)sample;
    }
    
    buffer_index = 0;
    buffer_ready = true;
    
    return 0;
}

void speaker_volume(int16_t* buffer, size_t num_samples, uint16_t volume_percent)
{
    if (volume_percent > 100)
    {
        volume_percent = 100;
    }
    
    float scale = volume_percent / 100.0f;
    
    for (size_t i = 0; i < num_samples; i++)
    {
        buffer[i] = (int16_t)(buffer[i] * scale);
    }
}

int speaker_test_blocking(void) {
    if (!buffer_ready) {
        return -1;
    }
    
    for (size_t i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        cyhal_dac_write(&dac_obj, audio_buffer[i]);
        cyhal_system_delay_us(20);
        // uint16_t test = cyhal_dac_read(&dac_obj);
    }
    
    buffer_ready = false;
    return 0;
}

void speaker_click(int16_t* buffer, size_t num_samples, int16_t amplitude) {
    for (size_t i = 0; i < num_samples; i++) {
        float decay = expf(-30.0f * i / (float)num_samples);
        buffer[i] = (int16_t)(amplitude * decay);
    }
}

void speaker_startup(int16_t* buffer, size_t buffer_size, int sample_rate) {
    float total_duration = 3.0f;
    int total_samples = (int)(total_duration * sample_rate);
    float dt = 1.0f / sample_rate;
    float PI = 3.14159265358979f;
    
    float phase = 0.0f;
    float frequency = 260.0f;
    
    float attack = 0.8f;
    float decay = 0.3f;
    float sustain = 0.7f;
    float release = 0.6f;
    
    float env_time = 0.0f;
    float amplitude = 0.0f;
    int released = 0;
    
    int samples_generated = 0;
    
    while (samples_generated < total_samples) {
        int chunk_size = buffer_size;
        if (samples_generated + chunk_size > total_samples) {
            chunk_size = total_samples - samples_generated;
        }
        
        for (int i = 0; i < chunk_size; i++) {
            env_time += dt;
            
            if (env_time > total_duration - release && !released) {
                released = 1;
            }
            
            if (!released) {
                if (env_time < attack) {
                    amplitude = env_time / attack;
                } else if (env_time < attack + decay) {
                    float progress = (env_time - attack) / decay;
                    amplitude = 1.0f - progress * (1.0f - sustain);
                } else {
                    amplitude = sustain;
                }
            } else {
                amplitude -= dt / release;
                if (amplitude < 0.0f) amplitude = 0.0f;
            }
            
            // Rich chime: fundamental + harmonics with different decay rates
            float harmonic_decay = env_time * 0.5f;  // Harmonics fade faster
            float h2_amp = 0.5f * expf(-harmonic_decay * 1.0f);
            float h3_amp = 0.25f * expf(-harmonic_decay * 2.0f);
            float h4_amp = 0.125f * expf(-harmonic_decay * 3.0f);
            
            float sample = sinf(phase)                    // Fundamental
                         + h2_amp * sinf(2.0f * phase)    // 2nd harmonic
                         + h3_amp * sinf(3.0f * phase)    // 3rd harmonic
                         + h4_amp * sinf(4.0f * phase);   // 4th harmonic
            
            // Normalize and apply envelope
            sample = (sample / (1.0f + h2_amp + h3_amp + h4_amp)) * amplitude;
            buffer[i] = (int16_t)(sample * 32767);
            
            phase += (2.0f * PI * frequency) / sample_rate;
            if (phase > 2.0f * PI) phase -= 2.0f * PI;
            
            float progress = (float)samples_generated / total_samples;
            frequency = 260.0f * powf(880.0f / 260.0f, progress);
        }

        
        speaker_write(buffer, chunk_size);
        speaker_test_blocking();
        
        // Update progress for frequency calculation
        samples_generated += chunk_size;
    }

    // Add a tiny delay if you want a gap between the chime and the coin
    // cyhal_system_delay_us(100000); // Optional: 0.1s pause for clarity
    // speaker_mario_coin(buffer, buffer_size, sample_rate);
}

void wall_hit_note(int16_t* buffer, size_t buffer_size, int sample_rate, float frequency) {
    
    float notes[] = {523.25f, 369.99f, 261.63f}; // C6, F#5, C5
    float duration = 0.05f; // Fast, staccato notes
    
    int num_notes = 3;
    float PI = 3.14159265358979f;
    float phase = 0.0f;
    
    for (int k = 0; k < num_notes; k++) {
        float freq = notes[k];
        int total_samples = (int)(duration * sample_rate);
        int samples_generated = 0;
        
        while (samples_generated < total_samples) {
            int chunk_size = buffer_size;
            if (samples_generated + chunk_size > total_samples) {
                chunk_size = total_samples - samples_generated;
            }
            
            for (int i = 0; i < chunk_size; i++) {
                // Square Wave for "8-bit damage" texture
                float sample = (phase < PI) ? 0.6f : -0.6f;
                
                // Add a slight decay to each note so they don't click
                float progress = (float)(samples_generated + i) / total_samples;
                sample *= (1.0f - progress * 0.5f); 
                
                buffer[i] = (int16_t)(sample * 32767);
                
                phase += (2.0f * PI * freq) / sample_rate;
                if (phase > 2.0f * PI) phase -= 2.0f * PI;
            }
            
            speaker_write(buffer, chunk_size);
            speaker_test_blocking();
            samples_generated += chunk_size;
        }
        // No delay between notes for a rapid cascade feel
    }
    
}



// Internal helper to play a specific frequency for a duration
// We make this static so it's private to this file
static void speaker_play_note(int16_t* buffer, size_t buffer_size, int sample_rate, float frequency, float duration_sec) {
    int total_samples = (int)(duration_sec * sample_rate);
    float dt = 1.0f / sample_rate;
    float PI = 3.14159265358979f;
    
    float phase = 0.0f;
    
    // ADSR Envelope for distinct notes
    // Short attack/release ensures notes don't bleed into each other
    float attack = 0.02f; 
    float decay = 0.05f;
    float sustain = 0.8f;
    float release = 0.05f; 
    
    float env_time = 0.0f;
    float amplitude = 0.0f;
    int released = 0;
    
    int samples_generated = 0;
    
    while (samples_generated < total_samples) {
        int chunk_size = buffer_size;
        if (samples_generated + chunk_size > total_samples) {
            chunk_size = total_samples - samples_generated;
        }
        
        for (int i = 0; i < chunk_size; i++) {
            env_time += dt;
            
            // Handle Envelope State
            if (env_time > duration_sec - release && !released) {
                released = 1;
            }
            
            if (!released) {
                if (env_time < attack) {
                    amplitude = env_time / attack;
                } else if (env_time < attack + decay) {
                    float progress = (env_time - attack) / decay;
                    amplitude = 1.0f - progress * (1.0f - sustain);
                } else {
                    amplitude = sustain;
                }
            } else {
                amplitude -= dt / release;
                if (amplitude < 0.0f) amplitude = 0.0f;
            }
            
            // Synthesis: Fundamental + Harmonics (Rich Sound)
            // This matches the style of your speaker_startup function
            float sample = sinf(phase)                    
                         + 0.5f * sinf(2.0f * phase)    
                         + 0.25f * sinf(3.0f * phase);   
            
            // Normalize (1 + 0.5 + 0.25 = 1.75) and apply amplitude
            sample = (sample / 1.75f) * amplitude;
            buffer[i] = (int16_t)(sample * 32767);
            
            phase += (2.0f * PI * frequency) / sample_rate;
            if (phase > 2.0f * PI) phase -= 2.0f * PI;
        }
        
        speaker_write(buffer, chunk_size);
        speaker_test_blocking();
        
        samples_generated += chunk_size;
    }
}



void speaker_mario(int16_t* buffer, size_t buffer_size, int sample_rate) {
    // --- Definitions for Octave 5 & 6 (Safe for 440Hz+ Speaker) ---
    float NOTE_G4  = 392.00f; // Borderline, but used in intro
    float NOTE_C5  = 523.25f;
    float NOTE_E5  = 659.25f;
    float NOTE_F5  = 698.46f;
    float NOTE_G5  = 783.99f;
    float NOTE_A5  = 880.00f;
    float NOTE_Bb5 = 932.33f;
    float NOTE_B5  = 987.77f;
    
    float NOTE_C6  = 1046.50f;
    float NOTE_D6  = 1174.66f;
    float NOTE_E6  = 1318.51f;
    float NOTE_F6  = 1396.91f;
    float NOTE_G6  = 1567.98f;
    float NOTE_A6  = 1760.00f;

    // ===========================
    // INTRO (Original)
    // ===========================
    
    // E E E
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.10f);
    cyhal_system_delay_us(50000);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.10f);
    cyhal_system_delay_us(100000);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.10f);
    cyhal_system_delay_us(100000);

    // C E
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C5, 0.10f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.10f);
    cyhal_system_delay_us(100000);

    // G - Low G
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G5, 0.20f);
    cyhal_system_delay_us(400000);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G4, 0.20f);
    cyhal_system_delay_us(400000); // Brief pause before main theme

    // ===========================
    // MAIN THEME (Transposed Up)
    // ===========================
    // We loop this section twice as per the song structure
    for(int k=0; k<2; k++) {
        
        // Phrase 1: C G E
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C6, 0.15f);
        cyhal_system_delay_us(100000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G5, 0.15f);
        cyhal_system_delay_us(100000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.15f);
        cyhal_system_delay_us(150000); // Longer Rest

        // Phrase 2: A B Bb A
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_A5, 0.15f);
        cyhal_system_delay_us(80000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_B5, 0.15f);
        cyhal_system_delay_us(80000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb5, 0.10f); // Semitone
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_A5, 0.15f);
        cyhal_system_delay_us(100000);

        // Phrase 3: G E G A F G
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G5, 0.12f); // Triplet feel
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E6, 0.12f);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G6, 0.12f);
        cyhal_system_delay_us(50000);
        
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_A6, 0.15f);
        cyhal_system_delay_us(50000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_F6, 0.12f);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G6, 0.12f);
        cyhal_system_delay_us(100000);

        // Phrase 4: E C D B
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E6, 0.15f);
        cyhal_system_delay_us(50000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C6, 0.15f);
        cyhal_system_delay_us(50000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_D6, 0.15f);
        cyhal_system_delay_us(50000);
        speaker_play_note(buffer, buffer_size, sample_rate, NOTE_B5, 0.15f);
        
        cyhal_system_delay_us(200000); // Rest between loops
    }
}

void speaker_mario_victory(int16_t* buffer, size_t buffer_size, int sample_rate) {
    // --- Phase 1: C Major Arpeggio ---
    // Note: G4 (392Hz) is slightly below spec, but acts as a quiet "pickup" note
    float NOTE_G4 = 392.00f; 
    float NOTE_C5 = 523.25f;
    float NOTE_E5 = 659.25f;
    float NOTE_G5 = 783.99f;
    float NOTE_C6 = 1046.50f;
    float NOTE_E6 = 1318.51f;
    float NOTE_G6 = 1567.98f;

    // The Run Up
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G4, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C6, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E6, 0.08f);
    
    // The Top
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G6, 0.20f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_E6, 0.20f);
    
    // --- Phase 2: Ab Major Arpeggio (Shifted) ---
    float NOTE_Ab4 = 415.30f;
    float NOTE_Eb5 = 622.25f;
    float NOTE_Ab5 = 830.61f;
    float NOTE_Eb6 = 1244.51f;
    float NOTE_Ab6 = 1661.22f;

    // The Run Up
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Ab4, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C5,  0.08f); // C is common
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Eb5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Ab5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C6,  0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Eb6, 0.08f);
    
    // The Top
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Ab6, 0.20f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Eb6, 0.20f);

    // --- Phase 3: Bb Major Arpeggio (Shifted) ---
    float NOTE_Bb4 = 466.16f; // Now fully within 440Hz spec!
    float NOTE_D5  = 587.33f;
    float NOTE_F5  = 698.46f;
    float NOTE_Bb5 = 932.33f;
    float NOTE_D6  = 1174.66f;
    float NOTE_F6  = 1396.91f;
    float NOTE_A6  = 1760.00f;
    float NOTE_B6  = 1975.53f;
    float NOTE_Bb6 = 1864.66f; // High clarity
    float NOTE_C7 = 2093.00f;


    // The Run Up
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb4, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_D5,  0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_F5,  0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb5, 0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_D6,  0.08f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_F6,  0.08f);
    
    // The Top
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb6, 0.20f); // High impact note
    // speaker_play_note(buffer, buffer_size, sample_rate, NOTE_F6,  0.08f);

    // speaker_play_note(buffer, buffer_size, sample_rate, NOTE_G6,  0.08f);
    // speaker_play_note(buffer, buffer_size, sample_rate, NOTE_A6,  0.20f);
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_B6, 0.08f); // High impact note

    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb6, 0.08f); // High impact note
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_Bb6, 0.08f); // High impact note


    // --- Final Note: C ---
    // Standard ending is often a high C held out
    // cyhal_system_delay_us(100000); // Brief dramatic pause
    speaker_play_note(buffer, buffer_size, sample_rate, NOTE_C7, 0.60f);
}



void speaker_mario_coin(int16_t* buffer, size_t buffer_size, int sample_rate) {
    // Frequencies: B5 -> E6
    float freq_1 = 987.77f;
    float freq_2 = 1318.51f;
    
    // Durations
    float dur_1 = 0.06f; // Very short first note
    float dur_2 = 0.40f; // Longer tail
    
    // Variables for generation
    float phase = 0.0f;
    float PI = 3.14159265358979f;
    float dt = 1.0f / sample_rate;
    int samples_generated = 0;
    
    // --- NOTE 1: The "Clink" (B5) ---
    // Square wave, constant volume, HARD STOP (no fade out)
    int total_samples_1 = (int)(dur_1 * sample_rate);
    
    while (samples_generated < total_samples_1) {
        int chunk_size = buffer_size;
        if (samples_generated + chunk_size > total_samples_1) {
            chunk_size = total_samples_1 - samples_generated;
        }
        
        for (int i = 0; i < chunk_size; i++) {
            // Square Wave Logic: High if phase < PI, Low if phase > PI
            // This creates the "8-bit" texture
            float sample = (phase < PI) ? 0.6f : -0.6f; 
            
            buffer[i] = (int16_t)(sample * 32767);
            
            phase += (2.0f * PI * freq_1) / sample_rate;
            if (phase > 2.0f * PI) phase -= 2.0f * PI;
        }
        
        speaker_write(buffer, chunk_size);
        speaker_test_blocking();
        samples_generated += chunk_size;
    }

    // --- NOTE 2: The "Shimmer" (E6) ---
    // Square wave, fast decay
    samples_generated = 0; // Reset for next note
    int total_samples_2 = (int)(dur_2 * sample_rate);
    
    while (samples_generated < total_samples_2) {
        int chunk_size = buffer_size;
        if (samples_generated + chunk_size > total_samples_2) {
            chunk_size = total_samples_2 - samples_generated;
        }
        
        for (int i = 0; i < chunk_size; i++) {
            // Linear Decay Envelope for the "ring out"
            float progress = (float)(samples_generated + i) / total_samples_2;
            float amplitude = 0.6f * (1.0f - progress); 
            
            // Square Wave again
            float sample = (phase < PI) ? amplitude : -amplitude;
            
            buffer[i] = (int16_t)(sample * 32767);
            
            phase += (2.0f * PI * freq_2) / sample_rate;
            if (phase > 2.0f * PI) phase -= 2.0f * PI;
        }
        
        speaker_write(buffer, chunk_size);
        speaker_test_blocking();
        samples_generated += chunk_size;
    }
}


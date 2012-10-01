#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <font.h>
#include <sine.h>

//////////////////////////////// timer //////////////////////////////// 
volatile uint16_t timerOvfCount;

void timerSetup()
{
    TCCR1A= 0;
    //~ TCCR1B= (1<<CS12) |  (1<<CS10);  // clkIO/1024 => ~15.625 timer ticks per millisecond
    TCCR1B= (1<<CS12);  // clkIO/256 => ~62.5 timer ticks per millisecond
    TIMSK1= (1<<TOIE1);
}

uint32_t timerRead()
{
    cli();
    uint32_t tick= (uint32_t(timerOvfCount)<<16) | (TCNT1);
    sei();
    return tick;
}

ISR(TIMER1_OVF_vect)
{
    timerOvfCount++;
}


//////////////////////////////// led  //////////////////////////////// 

void ledOn()
{
    PORTE|= (1<<6);
}

void ledOff()
{
    PORTE&= ~(1<<6);
}

//////////////////////////////// reset //////////////////////////////// 

void reset()
{
    wdt_disable();  
    wdt_enable(WDTO_15MS);
    while(true)
        ;
}

void resetToBootloader()
{
    // wire PD7 to RESET.
    DDRD= 1<<7;
    PORTD= 0;
}



//////////////////////////////// sin //////////////////////////////// 

uint8_t isin(uint16_t x)
{
    switch((x>>8)&3)
    {
        case 0:
            return sintab[x&0xFF];
        case 1:
            return sintab[255-(x&0xFF)];
        case 2:
            return 255-sintab[x&0xFF];
        default:    // 3
            return 255-sintab[255-(x&0xFF)];
    }
}

//////////////////////////////// adc //////////////////////////////// 

void adcSelectPot()
{
    ADMUX= (1<<REFS0) | 0b110;  // Vcc reference voltage, adc6 (F6)
    ADCSRB= 0;
}

// not functional
void adcSelectTemperatureSensor()
{
    ADMUX= (1<<REFS1) | (1<<REFS0) | 0b00111;
    ADCSRB= (1<<MUX5);
}

void adcSetup()
{
    ADCSRA= (1<<ADEN);

    DDRF= (1<<0) | (1<<4);
    PORTF= (1<<4);          // pot between F0 and F4
    
    adcSelectPot();
}

uint16_t adcRead()
{
    ADCSRA|= (1<<ADSC);
    while(ADCSRA & (1<<ADSC))
        ;
    return ADC;
}


//////////////////////////////// setup //////////////////////////////// 

void setup(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

    DDRE|= (1<<6);      // init led output
    DDRD= 0xff;         // lower 8 led pins
    DDRB= 0b1111;         // high 4 led pins
    PORTD= 0;
    
    timerSetup();
    adcSetup();
}

//////////////////////////////// display //////////////////////////////// 

// set lower 4 bits
void setLedsLo(uint8_t pins)
{
    PORTB= ~pins;
}

void setLedsHi(uint8_t pins)
{
    PORTD= ~pins;
}

uint8_t ledOffset= 0;
void setLeds(uint16_t values)
{
    values <<= ledOffset;
    setLedsLo(values&15);
    setLedsHi(values>>4);
}

void putCharRow(char ch, uint8_t row)
{
    unsigned char r= font[(ch-32)*8+row];
    setLeds(r);
}

//////////////////////////////// row stuff //////////////////////////////// 

struct dispfoobase
{
    // gets called before tick() is first run
    virtual void select()
    { }
    // gets called every frame to generate a row
    virtual void tick(uint32_t time, uint32_t rowcount)
    { }
};

struct display_text: dispfoobase
{
    const char *mytext;
    uint8_t textlen;
    uint16_t textrow;
    uint8_t current_rowbits;
    
    display_text(const char *txt)
    { settext(txt); }
    
    void settext(const char *txt)
    {
        mytext= txt; 
        textlen= strlen(mytext);
    }
    
    void update_rowbits()
    {
        uint8_t ch= textrow>>3, chrow= textrow&7;
        current_rowbits= pgm_read_byte_near( font + (mytext[ch%textlen]-32)*8 + chrow );
        textrow++;
    }
    
    void select()
    {
        textrow= 0;
        update_rowbits();
    }
    void tick(uint32_t time, uint32_t rowcount)
    {
        setLeds(current_rowbits);
        update_rowbits();
    }
};

struct display_sine: dispfoobase
{
    uint16_t phase;
    virtual void tick(uint32_t time, uint32_t rowcount)
    {
        phase+= isin(rowcount*3>>4)+1;
        uint16_t leds= 0;
        uint8_t s= isin(phase*3>>4);
        leds|= 1 << (s>>5);
        //~ leds|= 1 << ((s>>5) + ((s>>4)&1));
        //~ s= isin(time*550>>10);
        //~ leds|= 1 << (s>>5);
        //~ s= isin(rowcount*30);
        //~ leds|= 1 << (s>>5);
        setLeds(leds);
    }
};


struct display_bincount: dispfoobase
{
    virtual void select()
    {
    }
    virtual void tick(uint32_t time, uint32_t rowcount)
    {
        setLeds(rowcount>>3);
    }
};


struct display_font: dispfoobase
{
    virtual void tick(uint32_t time, uint32_t rowcount)
    {
        setLeds( pgm_read_byte_near(font + (rowcount%760)) );
    }
};

// not functional
struct display_temperature: display_text
{
    display_temperature(): display_text("")
    { }
    virtual void select()
    {
        static const char tempFmt[]= "  T: %d  ";
        static char tempTxt[20];
        adcSelectTemperatureSensor();
        for(int i= 0; i<10; i++)
            adcRead();
        snprintf(tempTxt, sizeof(tempTxt), tempFmt, adcRead());
        adcSelectPot();
        display_text::settext(tempTxt);
        display_text::select();
    }
};




//////////////////////////////// main //////////////////////////////// 

int main(void)
{
	setup();
    
	sei();
    
    display_text nyan= display_text("NYAN NYAN NYAN NYAN, Nyan nYan nyAn nyaN Nyan // ");
    display_sine sine= display_sine();
    display_text greet= display_text("outdoor geekend ersatzveranstaltung // c3d2 // musik, mnemonics und interwebs // jetzt auf einer wiese in deiner naehe // ");
    display_bincount bincount= display_bincount();
    display_font showfont= display_font();
    //~ display_temperature showtemp= display_temperature();
    struct { uint16_t time; dispfoobase *dispfoo; } story[]= 
    { 
        //~ { 2000, &showfont },
        { 2000, &sine },
        { 4000, &nyan },
        { 2000, &sine },
        { 15000, &greet },
        //~ { 5000, &sine },
        //~ { 5000, &bincount },
    };

    uint32_t rowcount= 0;
    uint8_t story_index= 0;
    uint32_t story_switch= 0;
    for (;;)
	{
        uint32_t time= timerRead();
        
        if(time > story_switch)
        {
            ++story_index;
            story_index%= (sizeof(story)/sizeof(story[0]));
            story[story_index].dispfoo->select();
            story_switch+= (uint32_t)(story[story_index].time)<<6;
        }
        
        story[story_index].dispfoo->tick(time, rowcount);
        
        //~ setLeds(0xff);
        
        //~ ledOffset= isin(time>>5) * 5 >> 8;
        
        //~ uint16_t delay= (adcRead() >> 3) + 20;
        uint16_t delay= 20;
        while(timerRead() < time+delay)
            ;
        
        rowcount++;
    }
}


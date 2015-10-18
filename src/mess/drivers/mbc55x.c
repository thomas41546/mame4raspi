/*
    drivers/mbc55x.c

    Machine driver for the Sanyo MBC-550 and MBC-555.

    Phill Harvey-Smith
    2011-01-29.


ToDo:
- Fix the sound
- Add serial uart

*/


#include "includes/mbc55x.h"


const unsigned char mbc55x_palette[SCREEN_NO_COLOURS][3] =
{
	/*normal brightness */
	{ 0x00,0x00,0x00 }, /* black */
	{ 0x00,0x00,0x80 }, /* blue */
	{ 0x00,0x80,0x00 }, /* green */
	{ 0x00,0x80,0x80 }, /* cyan */
	{ 0x80,0x00,0x00 }, /* red */
	{ 0x80,0x00,0x80 }, /* magenta */
	{ 0x80,0x80,0x00 }, /* yellow */
	{ 0x80,0x80,0x80 }, /* light grey */
};

static const floppy_interface mbc55x_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSSD_35,
	LEGACY_FLOPPY_OPTIONS_NAME(pc),
	"floppy_5_25",
	NULL
};

static ADDRESS_MAP_START(mbc55x_mem, AS_PROGRAM, 8, mbc55x_state)
	AM_RANGE( 0x00000, 0x0FFFF ) AM_RAMBANK(RAM_BANK00_TAG)
	AM_RANGE( 0x10000, 0x1FFFF ) AM_RAMBANK(RAM_BANK01_TAG)
	AM_RANGE( 0x20000, 0x2FFFF ) AM_RAMBANK(RAM_BANK02_TAG)
	AM_RANGE( 0x30000, 0x3FFFF ) AM_RAMBANK(RAM_BANK03_TAG)
	AM_RANGE( 0x40000, 0x4FFFF ) AM_RAMBANK(RAM_BANK04_TAG)
	AM_RANGE( 0x50000, 0x5FFFF ) AM_RAMBANK(RAM_BANK05_TAG)
	AM_RANGE( 0x60000, 0x6FFFF ) AM_RAMBANK(RAM_BANK06_TAG)
	AM_RANGE( 0x70000, 0x7FFFF ) AM_RAMBANK(RAM_BANK07_TAG)
	AM_RANGE( 0x80000, 0x8FFFF ) AM_RAMBANK(RAM_BANK08_TAG)
	AM_RANGE( 0x90000, 0x9FFFF ) AM_RAMBANK(RAM_BANK09_TAG)
	AM_RANGE( 0xA0000, 0xAFFFF ) AM_RAMBANK(RAM_BANK0A_TAG)
	AM_RANGE( 0xB0000, 0xBFFFF ) AM_RAMBANK(RAM_BANK0B_TAG)
	AM_RANGE( 0xC0000, 0xCFFFF ) AM_RAMBANK(RAM_BANK0C_TAG)
	AM_RANGE( 0xD0000, 0xDFFFF ) AM_RAMBANK(RAM_BANK0D_TAG)
	AM_RANGE( 0xE0000, 0xEFFFF ) AM_RAMBANK(RAM_BANK0E_TAG)
	AM_RANGE( 0xF0000, 0xF3FFF ) AM_RAMBANK(RED_PLANE_TAG)
	AM_RANGE( 0xF4000, 0xF7FFF ) AM_RAMBANK(BLUE_PLANE_TAG)
	AM_RANGE( 0xF8000, 0xFBFFF ) AM_NOP
	AM_RANGE( 0xFC000, 0xFFFFF ) AM_ROM AM_WRITENOP AM_REGION(MAINCPU_TAG, 0x0000) AM_MIRROR(0x002000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbc55x_io, AS_IO, 8, mbc55x_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0X0000, 0X0003) AM_READWRITE(mbcpic8259_r, mbcpic8259_w)
	AM_RANGE( 0x0008, 0x000F) AM_READWRITE(mbc55x_disk_r, mbc55x_disk_w)
	AM_RANGE( 0X0010, 0X0010) AM_READWRITE( vram_page_r, vram_page_w)
	AM_RANGE( 0x0018, 0x001F) AM_READWRITE( ppi8255_r, ppi8255_w)
	AM_RANGE( 0X0020, 0X0027) AM_READWRITE(mbcpit8253_r, mbcpit8253_w)
	AM_RANGE( 0x0028, 0x002B) AM_READWRITE( mbc55x_usart_r, mbc55x_usart_w)
	AM_RANGE( 0x0030, 0x0031) AM_DEVREADWRITE(VID_MC6845_NAME, mc6845_device, status_r, address_w )
	AM_RANGE( 0x0032, 0x0033) AM_DEVREADWRITE(VID_MC6845_NAME, mc6845_device, register_r, register_w )
	AM_RANGE( 0x0038, 0x003B) AM_READWRITE(mbc55x_kb_usart_r, mbc55x_kb_usart_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( mbc55x )
	PORT_START("KEY0") /* Key row 0 scancodes 00..07 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1")      PORT_CODE(KEYCODE_1)            PORT_CHAR('1')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2")      PORT_CODE(KEYCODE_2)            PORT_CHAR('2')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3")      PORT_CODE(KEYCODE_3)            PORT_CHAR('3')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4")      PORT_CODE(KEYCODE_4)            PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5")      PORT_CODE(KEYCODE_5)            PORT_CHAR('5')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6")      PORT_CODE(KEYCODE_6)            PORT_CHAR('6')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7")      PORT_CODE(KEYCODE_7)            PORT_CHAR('7')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8")      PORT_CODE(KEYCODE_8)            PORT_CHAR('8')

	PORT_START("KEY1") /* Key row 1 scancodes 08..0F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9")      PORT_CODE(KEYCODE_9)            PORT_CHAR('9')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0")      PORT_CODE(KEYCODE_0)            PORT_CHAR('0')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-")      PORT_CODE(KEYCODE_MINUS)        PORT_CHAR('-')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=")      PORT_CODE(KEYCODE_EQUALS)       PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BSLASH") PORT_CODE(KEYCODE_BACKSLASH)    PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q")      PORT_CODE(KEYCODE_Q)            PORT_CHAR('Q')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W")      PORT_CODE(KEYCODE_W)            PORT_CHAR('W')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E")      PORT_CODE(KEYCODE_E)            PORT_CHAR('E')

	PORT_START("KEY2") /* Key row 2 scancodes 10..17 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R")      PORT_CODE(KEYCODE_R)            PORT_CHAR('R')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T")      PORT_CODE(KEYCODE_T)            PORT_CHAR('T')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y")      PORT_CODE(KEYCODE_Y)            PORT_CHAR('Y')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U")      PORT_CODE(KEYCODE_U)            PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I")      PORT_CODE(KEYCODE_I)            PORT_CHAR('I')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O")      PORT_CODE(KEYCODE_O)            PORT_CHAR('O')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P")      PORT_CODE(KEYCODE_P)            PORT_CHAR('P')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[")      PORT_CODE(KEYCODE_OPENBRACE)    PORT_CHAR('[')


	PORT_START("KEY3") /* Key row 3 scancodes 18..1F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]")      PORT_CODE(KEYCODE_CLOSEBRACE)   PORT_CHAR(']')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")      PORT_CODE(KEYCODE_A)            PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S")      PORT_CODE(KEYCODE_S)            PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D")      PORT_CODE(KEYCODE_D)            PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F")      PORT_CODE(KEYCODE_F)            PORT_CHAR('F')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G")      PORT_CODE(KEYCODE_G)            PORT_CHAR('G')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H")      PORT_CODE(KEYCODE_H)            PORT_CHAR('H')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J")      PORT_CODE(KEYCODE_J)            PORT_CHAR('J')

	PORT_START("KEY4") /* Key row 4 scancodes 20..27 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K")      PORT_CODE(KEYCODE_K)            PORT_CHAR('K')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L")      PORT_CODE(KEYCODE_L)            PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";")      PORT_CODE(KEYCODE_COLON)        PORT_CHAR(';')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TILDE")  PORT_CODE(KEYCODE_TILDE)        PORT_CHAR('`')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("#")      PORT_CODE(KEYCODE_QUOTE)        PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER")  PORT_CODE(KEYCODE_ENTER)        PORT_CHAR(0x0D)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")      PORT_CODE(KEYCODE_Z)            PORT_CHAR('Z')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X")      PORT_CODE(KEYCODE_X)            PORT_CHAR('X')


	PORT_START("KEY5") /* Key row 5 scancodes 28..2F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C")      PORT_CODE(KEYCODE_C)            PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V")      PORT_CODE(KEYCODE_V)            PORT_CHAR('V')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B")      PORT_CODE(KEYCODE_B)            PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N")      PORT_CODE(KEYCODE_N)            PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M")      PORT_CODE(KEYCODE_M)            PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",")      PORT_CODE(KEYCODE_COMMA)        PORT_CHAR(',')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".")      PORT_CODE(KEYCODE_STOP)         PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/")      PORT_CODE(KEYCODE_SLASH)        PORT_CHAR('/')

	PORT_START("KEY6") /* Key row 6 scancodes 30..37 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE")  PORT_CODE(KEYCODE_SPACE)        PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

#if 0
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BACKSLASH") PORT_CODE(KEYCODE_BACKSLASH)    PORT_CHAR('\\')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT)       PORT_CHAR(UCHAR_SHIFT_1)

	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")   PORT_CODE(KEYCODE_LCONTROL)     PORT_CODE(KEYCODE_RCONTROL) // Ether control

	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)       PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PRSCR")  PORT_CODE(KEYCODE_ASTERISK)     PORT_CHAR('*')

	PORT_START("KEY7") /* Key row 7 scancodes 38..3F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALT")    PORT_CODE(KEYCODE_LALT)         PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS")   PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1")     PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2")     PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3")     PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4")     PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5")     PORT_CODE(KEYCODE_F5)

	PORT_START("KEY8") /* Key row 8 scancodes 40..47 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6")     PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7")     PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8")     PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F9")     PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F10")    PORT_CODE(KEYCODE_F10)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMLK")  PORT_CODE(KEYCODE_NUMLOCK)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SCRLK")  PORT_CODE(KEYCODE_SCRLOCK)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP7")    PORT_CODE(KEYCODE_7_PAD)        PORT_CHAR('7')

	PORT_START("KEY9") /* Key row 9 scancodes 48..4F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP8")    PORT_CODE(KEYCODE_8_PAD)        //PORT_CHAR('8')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP9")    PORT_CODE(KEYCODE_9_PAD)        //PORT_CHAR('9')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP-")    PORT_CODE(KEYCODE_MINUS_PAD)    //PORT_CHAR('-')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP4")    PORT_CODE(KEYCODE_4_PAD)        //PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP5")    PORT_CODE(KEYCODE_5_PAD)        //PORT_CHAR('5')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP6")    PORT_CODE(KEYCODE_6_PAD)        //PORT_CHAR('6')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP+")    PORT_CODE(KEYCODE_PLUS_PAD)     //PORT_CHAR('+')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP1")    PORT_CODE(KEYCODE_1_PAD)        //PORT_CHAR('1')

	PORT_START("KEY10") /* Key row 10 scancodes 50..57 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP2")    PORT_CODE(KEYCODE_2_PAD)        //PORT_CHAR('2')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP3")    PORT_CODE(KEYCODE_3_PAD)        //PORT_CHAR('3')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP0")    PORT_CODE(KEYCODE_0_PAD)        //PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP.")    PORT_CODE(KEYCODE_DEL_PAD)      //PORT_CHAR('.')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	//PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP5")    PORT_CODE(KEYCODE_5_PAD)        PORT_CHAR('5')
	//PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP6")    PORT_CODE(KEYCODE_6_PAD)        PORT_CHAR('6')
	//PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP+")    PORT_CODE(KEYCODE_PLUS_PAD)     PORT_CHAR('+')
	//PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP1")    PORT_CODE(KEYCODE_1_PAD)        PORT_CHAR('1')
#endif

	PORT_START(KEY_SPECIAL_TAG)
	PORT_BIT(KEY_BIT_LSHIFT,    IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT)       PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(KEY_BIT_RSHIFT,    IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)       PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(KEY_BIT_CTRL,      IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL")   PORT_CODE(KEYCODE_LCONTROL)     PORT_CODE(KEYCODE_RCONTROL) // Ether control
	PORT_BIT(KEY_BIT_GRAPH,     IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH")  PORT_CODE(KEYCODE_LALT)         PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END


void mbc55x_state::palette_init()
{
	int colourno;

	logerror("initializing palette\n");

	for ( colourno = 0; colourno < SCREEN_NO_COLOURS; colourno++ )
		palette_set_color_rgb(machine(), colourno, mbc55x_palette[colourno][RED], mbc55x_palette[colourno][GREEN], mbc55x_palette[colourno][BLUE]);
}


static MACHINE_CONFIG_START( mbc55x, mbc55x_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(MAINCPU_TAG, I8088, 3600000)
	MCFG_CPU_PROGRAM_MAP(mbc55x_mem)
	MCFG_CPU_IO_MAP(mbc55x_io)


	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_RAW_PARAMS(XTAL_14_31818MHz,896,0,300,262,0,200)
	MCFG_SCREEN_UPDATE_DEVICE(VID_MC6845_NAME, mc6845_device, screen_update)
	MCFG_SCREEN_VBLANK_DRIVER(mbc55x_state, screen_eof_mbc55x)

	MCFG_PALETTE_LENGTH(SCREEN_NO_COLOURS * 3)

//  MCFG_SCREEN_SIZE(650, 260)
//  MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)


	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
	MCFG_RAM_EXTRA_OPTIONS("128K,192K,256K,320K,384K,448K,512K,576K,640K")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO(MONO_TAG)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,MONO_TAG, 0.75)

	/* Devices */
	MCFG_I8251_ADD(I8251A_KB_TAG,mbc55x_i8251a_interface)
	MCFG_PIT8253_ADD( PIT8253_TAG, mbc55x_pit8253_config )
	MCFG_PIC8259_ADD( PIC8259_TAG, INPUTLINE(MAINCPU_TAG, INPUT_LINE_IRQ0), VCC, NULL )
	MCFG_I8255_ADD( PPI8255_TAG, mbc55x_ppi8255_interface )
	MCFG_MC6845_ADD(VID_MC6845_NAME, MC6845, SCREEN_TAG, XTAL_14_31818MHz/8, mb55x_mc6845_intf)

	/* Backing storage */
	MCFG_FD1793_ADD(FDC_TAG, mbc55x_wd17xx_interface )
	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(mbc55x_floppy_interface)

	/* Software list */
	MCFG_SOFTWARE_LIST_ADD("disk_list","mbc55x")
MACHINE_CONFIG_END


ROM_START( mbc55x )
	ROM_REGION( 0x4000, MAINCPU_TAG, 0 )

	ROM_SYSTEM_BIOS(0, "v120", "mbc55x BIOS v1.20 (1983)")
	ROMX_LOAD("mbc55x-v120.rom", 0x0000, 0x2000, CRC(b439b4b8) SHA1(6e8df0f3868e3fd0229a5c2720d6c01e46815cab), ROM_BIOS(1)  )
ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE INPUT   INIT  COMPANY  FULLNAME   FLAGS */
COMP( 1983, mbc55x,     0,      0,      mbc55x, mbc55x, driver_device, 0,   "Sanyo",  "MBC-55x",  0 /*GAME_NO_SOUND*/)

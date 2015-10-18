/***************************************************************************

Atari Sprint 8 video emulation

***************************************************************************/

#include "emu.h"
#include "includes/sprint8.h"


void sprint8_state::palette_init()
{
	int i;

	/* allocate the colortable */
	machine().colortable = colortable_alloc(machine(), 0x12);

	for (i = 0; i < 0x10; i++)
	{
		colortable_entry_set_value(machine().colortable, 2 * i + 0, 0x10);
		colortable_entry_set_value(machine().colortable, 2 * i + 1, i);
	}

	colortable_entry_set_value(machine().colortable, 0x20, 0x10);
	colortable_entry_set_value(machine().colortable, 0x21, 0x10);
	colortable_entry_set_value(machine().colortable, 0x22, 0x10);
	colortable_entry_set_value(machine().colortable, 0x23, 0x11);
}


void sprint8_state::set_pens(sprint8_state *state, colortable_t *colortable)
{
	int i;

	for (i = 0; i < 0x10; i += 8)
	{
		if (*m_team & 1)
		{
			colortable_palette_set_color(colortable, i + 0, MAKE_RGB(0xff, 0x00, 0x00)); /* red     */
			colortable_palette_set_color(colortable, i + 1, MAKE_RGB(0x00, 0x00, 0xff)); /* blue    */
			colortable_palette_set_color(colortable, i + 2, MAKE_RGB(0xff, 0xff, 0x00)); /* yellow  */
			colortable_palette_set_color(colortable, i + 3, MAKE_RGB(0x00, 0xff, 0x00)); /* green   */
			colortable_palette_set_color(colortable, i + 4, MAKE_RGB(0xff, 0x00, 0xff)); /* magenta */
			colortable_palette_set_color(colortable, i + 5, MAKE_RGB(0xe0, 0xc0, 0x70)); /* puce    */
			colortable_palette_set_color(colortable, i + 6, MAKE_RGB(0x00, 0xff, 0xff)); /* cyan    */
			colortable_palette_set_color(colortable, i + 7, MAKE_RGB(0xff, 0xaa, 0xaa)); /* pink    */
		}
		else
		{
			colortable_palette_set_color(colortable, i + 0, MAKE_RGB(0xff, 0x00, 0x00)); /* red     */
			colortable_palette_set_color(colortable, i + 1, MAKE_RGB(0x00, 0x00, 0xff)); /* blue    */
			colortable_palette_set_color(colortable, i + 2, MAKE_RGB(0xff, 0x00, 0x00)); /* red     */
			colortable_palette_set_color(colortable, i + 3, MAKE_RGB(0x00, 0x00, 0xff)); /* blue    */
			colortable_palette_set_color(colortable, i + 4, MAKE_RGB(0xff, 0x00, 0x00)); /* red     */
			colortable_palette_set_color(colortable, i + 5, MAKE_RGB(0x00, 0x00, 0xff)); /* blue    */
			colortable_palette_set_color(colortable, i + 6, MAKE_RGB(0xff, 0x00, 0x00)); /* red     */
			colortable_palette_set_color(colortable, i + 7, MAKE_RGB(0x00, 0x00, 0xff)); /* blue    */
		}
	}

	colortable_palette_set_color(colortable, 0x10, MAKE_RGB(0x00, 0x00, 0x00));
	colortable_palette_set_color(colortable, 0x11, MAKE_RGB(0xff, 0xff, 0xff));
}


TILE_GET_INFO_MEMBER(sprint8_state::get_tile_info1)
{
	UINT8 code = m_video_ram[tile_index];

	int color = 0;

	if ((code & 0x30) != 0x30) /* ? */
		color = 17;
	else
	{
		if ((tile_index + 1) & 0x010)
			color |= 1;

		if (code & 0x80)
			color |= 2;

		if (tile_index & 0x200)
			color |= 4;

	}

	SET_TILE_INFO_MEMBER(code >> 7, code, color, (code & 0x40) ? (TILE_FLIPX | TILE_FLIPY) : 0);
}


TILE_GET_INFO_MEMBER(sprint8_state::get_tile_info2)
{
	UINT8 code = m_video_ram[tile_index];

	int color = 0;

	if ((code & 0x38) != 0x28)
		color = 16;
	else
		color = 17;

	SET_TILE_INFO_MEMBER(code >> 7, code, color, (code & 0x40) ? (TILE_FLIPX | TILE_FLIPY) : 0);
}


WRITE8_MEMBER(sprint8_state::sprint8_video_ram_w)
{
	m_video_ram[offset] = data;
	m_tilemap1->mark_tile_dirty(offset);
	m_tilemap2->mark_tile_dirty(offset);
}


void sprint8_state::video_start()
{
	m_screen->register_screen_bitmap(m_helper1);
	m_screen->register_screen_bitmap(m_helper2);

	m_tilemap1 = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(sprint8_state::get_tile_info1),this), TILEMAP_SCAN_ROWS, 16, 8, 32, 32);
	m_tilemap2 = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(sprint8_state::get_tile_info2),this), TILEMAP_SCAN_ROWS, 16, 8, 32, 32);

	m_tilemap1->set_scrolly(0, +24);
	m_tilemap2->set_scrolly(0, +24);
}


void sprint8_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		UINT8 code = m_pos_d_ram[i];

		int x = m_pos_h_ram[i];
		int y = m_pos_v_ram[i];

		if (code & 0x80)
			x |= 0x100;

		drawgfx_transpen(bitmap, cliprect, machine().gfx[2],
			code ^ 7,
			i,
			!(code & 0x10), !(code & 0x08),
			496 - x, y - 31, 0);
	}
}


TIMER_CALLBACK_MEMBER(sprint8_state::sprint8_collision_callback)
{
	sprint8_set_collision(param);
}


UINT32 sprint8_state::screen_update_sprint8(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	set_pens(this, machine().colortable);
	m_tilemap1->draw(screen, bitmap, cliprect, 0, 0);
	draw_sprites(bitmap, cliprect);
	return 0;
}


void sprint8_state::screen_eof_sprint8(screen_device &screen, bool state)
{
	// rising edge
	if (state)
	{
		int x;
		int y;
		const rectangle &visarea = m_screen->visible_area();

		m_tilemap2->draw(screen, m_helper2, visarea, 0, 0);

		m_helper1.fill(0x20, visarea);

		draw_sprites(m_helper1, visarea);

		for (y = visarea.min_y; y <= visarea.max_y; y++)
		{
			const UINT16* p1 = &m_helper1.pix16(y);
			const UINT16* p2 = &m_helper2.pix16(y);

			for (x = visarea.min_x; x <= visarea.max_x; x++)
				if (p1[x] != 0x20 && p2[x] == 0x23)
					machine().scheduler().timer_set(m_screen->time_until_pos(y + 24, x),
							timer_expired_delegate(FUNC(sprint8_state::sprint8_collision_callback),this),
							colortable_entry_get_value(machine().colortable, p1[x]));
		}
	}
}

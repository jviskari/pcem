/*IBM VGA emulation*/
#include <stdlib.h>
#include "ibm.h"
#include "device.h"
#include "io.h"
#include "video.h"
#include "vid_svga.h"
#include "vid_vga.h"

typedef struct vga_t
{
        svga_t svga;
} vga_t;

void vga_out(uint16_t addr, uint8_t val, void *p)
{
        vga_t *vga = (vga_t *)p;
        svga_t *svga = &vga->svga;
        uint8_t old;
        
        pclog("vga_out : %04X %02X  %04X:%04X  %02X  %i\n", addr, val, CS,pc, ram[0x489], ins);
                
        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;

        switch (addr)
        {
                case 0x3D4:
                svga->crtcreg = val & 0x1f;
                return;
                case 0x3D5:
                if (svga->crtcreg <= 7 && svga->crtc[0x11] & 0x80) return;
                old = svga->crtc[svga->crtcreg];
                svga->crtc[svga->crtcreg] = val;
                if (old != val)
                {
                        if (svga->crtcreg < 0xe || svga->crtcreg > 0x10)
                        {
                                fullchange = changeframecount;
                                svga_recalctimings(svga);
                        }
                }
                break;
        }
        svga_out(addr, val, svga);
}

uint8_t vga_in(uint16_t addr, void *p)
{
        vga_t *vga = (vga_t *)p;
        svga_t *svga = &vga->svga;
        uint8_t temp;

        if (addr != 0x3da) pclog("vga_in : %04X ", addr);
                
        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;
             
        switch (addr)
        {
                case 0x3D4:
                temp = svga->crtcreg;
                break;
                case 0x3D5:
                temp = svga->crtc[svga->crtcreg];
                break;
                default:
                temp = svga_in(addr, svga);
                break;
        }
        if (addr != 0x3da) pclog("%02X  %04X:%04X\n", temp, CS,pc);
        return temp;
}

void *vga_init()
{
        vga_t *vga = malloc(sizeof(vga_t));
        memset(vga, 0, sizeof(vga_t));

        svga_init(&vga->svga, vga, 1 << 18, /*256kb*/
                   NULL,
                   vga_in, vga_out,
                   NULL);

        io_sethandler(0x03c0, 0x0020, vga_in, NULL, NULL, vga_out, NULL, NULL, vga);

        vga->svga.bpp = 8;
        vga->svga.miscout = 1;
        
        return vga;
}

void vga_close(void *p)
{
        vga_t *vga = (vga_t *)p;

        svga_close(&vga->svga);
        
        free(vga);
}

void vga_speed_changed(void *p)
{
        vga_t *vga = (vga_t *)p;
        
        svga_recalctimings(&vga->svga);
}

device_t vga_device =
{
        "VGA",
        vga_init,
        vga_close,
        vga_speed_changed,
        svga_add_status_info
};
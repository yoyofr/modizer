/*
 * @file    line6.c
 * @brief   68k simulator generated by gen68
 * @date    2014-07-03
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2016 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Line 6: Bcc/BSR/BRA */

DECL_LINE68(line600)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  if (!reg0)
    pc += get_nextw(); /* .W */
  else
    pc += reg0;        /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line601)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0  +8;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line602)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +16;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line603)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +24;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line604)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +32;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line605)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +40;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line606)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +48;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line607)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +56;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line608)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +64;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line609)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +72;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60A)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +80;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60B)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +88;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60C)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +96;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60D)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+104;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60E)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+112;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line60F)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+120;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line610)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-128;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line611)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-120;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line612)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-112;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line613)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-104;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line614)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -96;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line615)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -88;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line616)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -80;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line617)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -72;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line618)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -64;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line619)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -56;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61A)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -48;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61B)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -40;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61C)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -32;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61D)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -24;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61E)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -16;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line61F)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0  -8;      /* .B */
  BCC(pc,(reg9<<1)+0);
}

DECL_LINE68(line620)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  if (!reg0)
    pc += get_nextw(); /* .W */
  else
    pc += reg0;        /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line621)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0  +8;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line622)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +16;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line623)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +24;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line624)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +32;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line625)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +40;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line626)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +48;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line627)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +56;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line628)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +64;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line629)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +72;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62A)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +80;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62B)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +88;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62C)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 +96;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62D)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+104;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62E)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+112;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line62F)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0+120;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line630)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-128;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line631)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-120;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line632)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-112;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line633)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0-104;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line634)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -96;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line635)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -88;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line636)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -80;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line637)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -72;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line638)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -64;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line639)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -56;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63A)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -48;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63B)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -40;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63C)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -32;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63D)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -24;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63E)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0 -16;      /* .B */
  BCC(pc,(reg9<<1)+1);
}

DECL_LINE68(line63F)
{
  /* Bcc or BSR */
  uint68_t pc = REG68.pc;
  pc += reg0  -8;      /* .B */
  BCC(pc,(reg9<<1)+1);
}


ERROR ON
UMASK 022
# SETEUID 0 0
IMG_OPEN lorem_sm.txt lorem_sm
IMG_OPEN lorem_md.txt lorem_md


MKDIR Kah 0755
CHMOD Kah 0750
CREATE Kah/Foo.blank 0640
DD /dev/lorem_sm Kah/Lorem.txt
DD /dev/lorem_sm Kah/Lorem2.txt
CHMOD Kah/Lorem.txt 0444
UNLINK Kah/Lorem2.txt


MKDIR Kah/Zig/
DD /dev/zero Kah/Zig/index.w 32 1K
OPEN Kah/Zig/index.w WRONLY 0 @fidx

MKDIR Kah/Zig/00
MKDIR Kah/Zig/01
DD /dev/random Kah/Zig/01/A.w 32 1K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/02
MKDIR Kah/Zig/03
MKDIR Kah/Zig/04
MKDIR Kah/Zig/05
DD /dev/random Kah/Zig/05/B.w 32 2K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/06
MKDIR Kah/Zig/07
DD /dev/random Kah/Zig/07/C.w 32 4K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/08
MKDIR Kah/Zig/09
MKDIR Kah/Zig/0A
MKDIR Kah/Zig/0B
MKDIR Kah/Zig/0C
MKDIR Kah/Zig/0D
MKDIR Kah/Zig/0E
MKDIR Kah/Zig/0F
DD /dev/random Kah/Zig/0F/D.w 32 8K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K


MKDIR Kah/Zig/10
MKDIR Kah/Zig/11
MKDIR Kah/Zig/12
MKDIR Kah/Zig/13
DD /dev/random Kah/Zig/13/E.w 32 16K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/14
MKDIR Kah/Zig/15
MKDIR Kah/Zig/16
MKDIR Kah/Zig/17
DD /dev/random Kah/Zig/17/F.w 32 32K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/18
MKDIR Kah/Zig/19
MKDIR Kah/Zig/1A
MKDIR Kah/Zig/1B
MKDIR Kah/Zig/1C
MKDIR Kah/Zig/1D
MKDIR Kah/Zig/1E
MKDIR Kah/Zig/1F

MKDIR Kah/Zig/20
MKDIR Kah/Zig/21
MKDIR Kah/Zig/22
MKDIR Kah/Zig/23
MKDIR Kah/Zig/24
MKDIR Kah/Zig/25
DD /dev/random Kah/Zig/25/G.w 32 64K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/26
MKDIR Kah/Zig/27
MKDIR Kah/Zig/28
MKDIR Kah/Zig/29
MKDIR Kah/Zig/2A
MKDIR Kah/Zig/2B
MKDIR Kah/Zig/2C
MKDIR Kah/Zig/2D
MKDIR Kah/Zig/2E
MKDIR Kah/Zig/2F

MKDIR Kah/Zig/30
MKDIR Kah/Zig/31
MKDIR Kah/Zig/32
MKDIR Kah/Zig/33
MKDIR Kah/Zig/34
MKDIR Kah/Zig/35
MKDIR Kah/Zig/36
MKDIR Kah/Zig/37
MKDIR Kah/Zig/38
MKDIR Kah/Zig/39
DD /dev/random Kah/Zig/39/H.w 32 128K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/3A
MKDIR Kah/Zig/3B
MKDIR Kah/Zig/3C
MKDIR Kah/Zig/3D
MKDIR Kah/Zig/3E
MKDIR Kah/Zig/3F

MKDIR Kah/Zig/40
MKDIR Kah/Zig/41
MKDIR Kah/Zig/42
MKDIR Kah/Zig/43
DD /dev/random Kah/Zig/43/I.w 32 256K
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
DD /dev/random Kah/Zig/43/J.w 32 512
READ @buf /dev/random 1K
WRITE @buf @fidx 1K
MKDIR Kah/Zig/44
MKDIR Kah/Zig/45
DD /dev/random Kah/Zig/45/K.w 32 512
READ @buf /dev/random 1K
WRITE @buf @fidx 1K

CLOSE @fidx
RMBUF @buf


MKDIR Kah/Sto 0755
DD /dev/lorem_md Kah/Sto/Lorem.txt 1K 1.44M


MKDIR Kah/Bnz 0755
DD /dev/random Kah/Bnz/sample_a1.txt 64 64
DD /dev/random Kah/Bnz/sample_b1.txt 64 64
DD /dev/random Kah/Bnz/sample_c1.txt 64 64
DD /dev/random Kah/Bnz/sample_d1.txt 64 64
DD /dev/random Kah/Bnz/sample_e1.txt 64 64
DD /dev/random Kah/Bnz/sample_f1.txt 64 64
DD /dev/random Kah/Bnz/sample_g1.txt 64 64
DD /dev/random Kah/Bnz/sample_h1.txt 64 64
DD /dev/random Kah/Bnz/sample_i1.txt 64 64
DD /dev/random Kah/Bnz/sample_j1.txt 64 64
DD /dev/random Kah/Bnz/sample_k1.txt 64 64
DD /dev/random Kah/Bnz/sample_l1.txt 64 64
DD /dev/random Kah/Bnz/sample_m1.txt 64 64
DD /dev/random Kah/Bnz/sample_n1.txt 64 64
DD /dev/random Kah/Bnz/sample_o1.txt 64 64
DD /dev/random Kah/Bnz/sample_p1.txt 64 64
DD /dev/random Kah/Bnz/sample_q1.txt 64 64
DD /dev/random Kah/Bnz/sample_r1.txt 64 64
DD /dev/random Kah/Bnz/sample_s1.txt 64 64
DD /dev/random Kah/Bnz/sample_t1.txt 64 64
DD /dev/random Kah/Bnz/sample_u1.txt 64 64
DD /dev/random Kah/Bnz/sample_v1.txt 64 64
DD /dev/random Kah/Bnz/sample_w1.txt 64 64
DD /dev/random Kah/Bnz/sample_x1.txt 64 64
DD /dev/random Kah/Bnz/sample_y1.txt 64 64
DD /dev/random Kah/Bnz/sample_z1.txt 64 64
DD /dev/random Kah/Bnz/sample_a2.txt 64 64
DD /dev/random Kah/Bnz/sample_b2.txt 64 64
DD /dev/random Kah/Bnz/sample_c2.txt 64 64
DD /dev/random Kah/Bnz/sample_d2.txt 64 64
DD /dev/random Kah/Bnz/sample_e2.txt 64 64
DD /dev/random Kah/Bnz/sample_f2.txt 64 64
DD /dev/random Kah/Bnz/sample_g2.txt 64 64
DD /dev/random Kah/Bnz/sample_h2.txt 64 64
DD /dev/random Kah/Bnz/sample_i2.txt 64 64
DD /dev/random Kah/Bnz/sample_j2.txt 64 64
DD /dev/random Kah/Bnz/sample_k2.txt 64 64
DD /dev/random Kah/Bnz/sample_l2.txt 64 64
DD /dev/random Kah/Bnz/sample_m2.txt 64 64
DD /dev/random Kah/Bnz/sample_n2.txt 64 64
DD /dev/random Kah/Bnz/sample_o2.txt 64 64
DD /dev/random Kah/Bnz/sample_p2.txt 64 64
DD /dev/random Kah/Bnz/sample_q2.txt 64 64
DD /dev/random Kah/Bnz/sample_r2.txt 64 64
DD /dev/random Kah/Bnz/sample_s2.txt 64 64
DD /dev/random Kah/Bnz/sample_t2.txt 64 64
DD /dev/random Kah/Bnz/sample_u2.txt 64 64
DD /dev/random Kah/Bnz/sample_v2.txt 64 64
DD /dev/random Kah/Bnz/sample_w2.txt 64 64
DD /dev/random Kah/Bnz/sample_x2.txt 64 64
DD /dev/random Kah/Bnz/sample_y2.txt 64 64
DD /dev/random Kah/Bnz/sample_z2.txt 64 64
UNLINK Kah/Bnz/sample_c1.txt
UNLINK Kah/Bnz/sample_e1.txt
UNLINK Kah/Bnz/sample_m2.txt
UNLINK Kah/Bnz/sample_f1.txt
UNLINK Kah/Bnz/sample_j1.txt
UNLINK Kah/Bnz/sample_k1.txt
UNLINK Kah/Bnz/sample_d2.txt
UNLINK Kah/Bnz/sample_a2.txt
UNLINK Kah/Bnz/sample_b2.txt
UNLINK Kah/Bnz/sample_n2.txt
UNLINK Kah/Bnz/sample_y2.txt
UNLINK Kah/Bnz/sample_n1.txt
UNLINK Kah/Bnz/sample_p2.txt
UNLINK Kah/Bnz/sample_p1.txt
UNLINK Kah/Bnz/sample_q1.txt
UNLINK Kah/Bnz/sample_o2.txt
DD /dev/random Kah/Bnz/sample_a3.txt 64 64
DD /dev/random Kah/Bnz/sample_b3.txt 64 64
DD /dev/random Kah/Bnz/sample_c3.txt 64 64
DD /dev/random Kah/Bnz/sample_d3.txt 64 64
DD /dev/random Kah/Bnz/sample_e3.txt 64 64
DD /dev/random Kah/Bnz/sample_f3.txt 64 64
DD /dev/random Kah/Bnz/sample_g3.txt 64 64
DD /dev/random Kah/Bnz/sample_h3.txt 64 64
DD /dev/random Kah/Bnz/sample_i3.txt 64 64
DD /dev/random Kah/Bnz/sample_j3.txt 64 64
DD /dev/random Kah/Bnz/sample_k3.txt 64 64
DD /dev/random Kah/Bnz/sample_l3.txt 64 64
DD /dev/random Kah/Bnz/sample_m3.txt 64 64
DD /dev/random Kah/Bnz/sample_n3.txt 64 64
DD /dev/random Kah/Bnz/sample_o3.txt 64 64
DD /dev/random Kah/Bnz/sample_p3.txt 64 64
DD /dev/random Kah/Bnz/sample_q3.txt 64 64
DD /dev/random Kah/Bnz/sample_r3.txt 64 64
DD /dev/random Kah/Bnz/sample_s3.txt 64 64
DD /dev/random Kah/Bnz/sample_t3.txt 64 64
DD /dev/random Kah/Bnz/sample_u3.txt 64 64
DD /dev/random Kah/Bnz/sample_v3.txt 64 64
DD /dev/random Kah/Bnz/sample_w3.txt 64 64
DD /dev/random Kah/Bnz/sample_x3.txt 64 64
DD /dev/random Kah/Bnz/sample_y3.txt 64 64
DD /dev/random Kah/Bnz/sample_z3.txt 64 64
UNLINK Kah/Bnz/sample_j3.txt
UNLINK Kah/Bnz/sample_m1.txt
UNLINK Kah/Bnz/sample_d3.txt
UNLINK Kah/Bnz/sample_b3.txt
UNLINK Kah/Bnz/sample_a3.txt
UNLINK Kah/Bnz/sample_n3.txt
UNLINK Kah/Bnz/sample_y3.txt

MKDIR Kah/Lrz
DD /dev/lorem_sm Kah/Foo.txt
RENAME Kah/Foo.txt Kah/Bar.txt
DD /dev/lorem_sm Kah/Foo.txt
RENAME Kah/Foo.txt Kah/Lrz/Foo.txt


ls Kah/Bnz

# SYMLINK Sto/Lorem.txt Kah/Blaz 
# SYMLINK Kah/Lorem.txt Kah/Gnee 

# MKDIR Dru
# LINK Kah/Lorem.txt Dru/Poo 






$fn=60;

base();
translate([11, 12, 9.5]) pcb();
translate([0, 0, 3.5]) lid();

module lid() {
    difference() {
        union() {
            difference() {
                cube([100, 100, 29.1]);
                translate([1, 1, -0.4]) cube([98, 98, 28]);
            }
            for(i = [[5, 5.5, 6.75], [5, 94.5, 6.75], [95, 5.5, 6.75], [95, 94.5, 6.75]]) {
                translate(i) union() {
                    cylinder(r=3.5, h=22);
                    translate([0, 0, 17]) cylinder(r1=3.5, r2=5, h=4);
                    translate([-5, -0.5, 0]) cube([10, 1, 22]);
                    translate([-0.5, -5, 0]) cube([1, 10, 22]);
                }
            }
            translate([13.75, 0, 0]) hull() {
                cube([32.9, 6, 19]);
                translate([0, 0, 26.6]) cube([32.9, 1, 1]);
            }
            translate([0, 67.15, 0]) hull() {
                cube([6, 17.66, 19]);
                translate([0, 0, 26.6]) cube([1, 17.66, 1]);
            }
        }
        union() {
            //salidas cables output
            for(i= [17.5:5.08:45]) {
                translate([i, -1, 0]) hull() {
                    rotate([-90, 0, 0]) cylinder(r=2.75, h=8);
                    translate([0, 0, 15]) rotate([-90, 0, 0]) cylinder(r=2.75, h=8);
                }
            }
            //salida cables input
            for(i= [70.9:5.08:85]) {
                translate([-1, i, 0]) hull() {
                    rotate([0, 90, 0]) cylinder(r=2.75, h=8);
                    translate([0, 0, 15]) rotate([0, 90, 0]) cylinder(r=2.75, h=8);
                }
            }
            //taladros torres
            for(i = [[5, 5.5, 6], [5, 94.5, 6], [95, 5.5, 6], [95, 94.5, 6]]) {
                translate(i) cylinder(r=1.5, h=24);
                translate(i) translate([0, 0, 5]) cylinder(r=2.9, h=24);
            }
            //antena
            translate([98, 23, 0]) hull() {
                rotate([0, 90, 0]) cylinder(r=2, h=3);
                translate([0, 0, 15]) rotate([0, 90, 0]) cylinder(r=2, h=3);
            }
            //simbolo peligro
            translate([27, 30, 28.1]) linear_extrude(height=2) scale(0.25) import("hvcaution-sign.dxf");
        }
    }
}

module base() {
    difference() {
        union() {
            //base
            cube([100, 100, 3]);
            //contorno PCB
            translate([9.5, 10.5, 0]) difference() {
                cube([81, 79, 13]);
                translate([1.5, 1.5, -1]) cube([78, 76, 15]);
            }
            //soportes PCB
            union() {
                translate([76, 11.5, 3]) cube([10, 2, 6.5]); //laterial inferior
                translate([10.5, 15, 3]) cube([2, 10, 6.5]); //lateral izquierdo
                translate([14, 86.5, 3]) cube([10, 2, 6.5]); //lateral superior
                translate([87.5, 75, 3]) cube([2, 10, 6.5]); //lateral derecho
                translate([15, 11.5, 3]) cube([30.5, 2, 6.5]); //refuerzo clemero salida
                translate([10.5, 68, 3]) cube([2, 16, 6.5]); //refuerzo clemero entrada
            }
            //pestañas
            union() {
                //lateral inferior
                translate([76, 9, 3]) cube([10,2,10]);
                translate([76, 10.5, 10]) hull() {
                    cube([10, 1, 2]);
                    translate([0, 0, 1.75]) cube([10, 2.5, 1.25]);
                }
                //lateral izquierdo
                translate([8, 15, 3]) cube([2, 10, 10]);
                translate([9.5, 15, 10]) hull() {
                    cube([1, 10, 2]);
                    translate([0, 0, 1.75]) cube([2.5, 10, 1.25]);
                }
                //lateral superior
                translate([14, 89.5, 3]) cube([10, 2, 10]);
                translate([14, 88.5, 10]) hull() {
                    cube([10, 1, 2]);
                    translate([0, -1.5, 1.75]) cube([10, 2.5, 1.25]);
                }
                //lateral derecho
                translate([90.5, 75, 3]) cube([2, 10, 10]);
                translate([89.5, 75, 10]) hull() {
                    cube([1, 10, 2]);
                    translate([-1.5, 0, 1.75]) cube([2.5, 10, 1.25]);
                }
            }
            //torres
            for(i = [[5, 5.5, 2], [5, 94.5, 2], [95, 5.5, 2], [95, 94.5, 2]]) {
                translate(i) cylinder(r=3.5, h=8);
            }
            translate([50, 50, 3]) cylinder(r1=8, r2=5, h=4);
        }
        union() {
            //cortes pestañas
            union() {
                translate([75, 10, 3]) cube([1, 4, 11]);
                translate([86, 10, 3]) cube([1, 4, 11]);
                translate([9, 14, 3]) cube([4, 1, 11]);
                translate([9, 25, 3]) cube([4, 1, 11]);
                translate([13, 86, 3]) cube([1, 4, 11]);
                translate([24, 86, 3]) cube([1, 4, 11]);
                translate([87, 74, 3]) cube([4, 1, 11]);
                translate([87, 85, 3]) cube([4, 1, 11]);
                
            }
            //taladros torres
            for(i = [[5, 5.5, -1], [5, 94.5, -1], [95, 5.5, -1], [95, 94.5, -1]]) {
                translate(i) cylinder(r=1.25, h=12);
            }
            //fijación pared
            translate([50, 50, -1]) cylinder(r=2, h=10);
            translate([50, 50, 3.1]) cylinder(r1=2, r2=4, h=4);
        }
    }
}

module pcb() {
    color("lightgreen") cube([78, 76, 1.6]); //PCB
    color("lightblue") translate([2, 2, -6]) cube([73, 72, 6]); //cableado
    union() {
        color("green") translate([4, 2, 1.6]) cube([30.5, 10, 14]); //output
        color("green") translate([4, 56, 1.6]) cube([10, 16, 14]); //input
        color("darkgrey") translate([18, 53, 1.6]) cube([34, 20, 16]); //acdc
        color("orange") translate([7, 14, 1.6]) cube([12, 29, 16]); //relay1
        color("orange") translate([22, 14, 1.6]) cube([12, 29, 16]); //relay2
        color("blue") translate([54, 31, 1.6]) cube([18, 45, 19]); //ardunano
        color("blue") translate([41, 2.5, 1.6]) cube([36, 28, 13]); //xbee
    }
}
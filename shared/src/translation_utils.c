#include "../headers/shared.h"

char* traducir_instruccion_string(t_instruccion* instr){
    instr_code cop = instr->codigo_operacion;
    operando op1 = instr->parametros[0];
    operando op2 = instr->parametros[1];
    switch(cop){
        case SET:       return string_from_format("%-8s %-10s %5d","SET",traducir_registro_cpu(op1),op2);
	    case ADD:       return string_from_format("%-8s %-10s %5s","ADD",traducir_registro_cpu(op1),traducir_registro_cpu(op2));
	    case MOV_IN:    return string_from_format("%-8s %-10s %5d","MOV_IN",traducir_registro_cpu(op1),op2);
	    case MOV_OUT:   return string_from_format("%-8s %-10d %5s","MOV_OUT",op1,traducir_registro_cpu(op2));
	    case IO:        if(op1==PANTALLA || op1==TECLADO) return string_from_format("%-8s %-10s %5s","I/O",traducir_dispositivo(op1),traducir_registro_cpu(op2));
                        else return string_from_format("%-8s %-10s %5d","I/O",traducir_dispositivo(op1),op2);
	    case EXIT:      return string_from_format("%-8s %-10d %5d","EXIT",op1,op2);
        default: return string_from_format("ERROR - COP INVALIDO: %d",cop);
    }
}

char* traducir_registro_cpu(registro_cpu registro){
    switch(registro){
        case AX: return "AX";
        case BX: return "BX";
        case CX: return "CX";
        case DX: return "DX";
        default: return "ERROR - REGISTRO INVALIDO";
    }
}

char* traducir_dispositivo(dispositivo disp){
    if(disp < PANTALLA) return lista_dispositivos[disp];
    else{
        if(disp == PANTALLA) return "PANTALLA";
        else if(disp == TECLADO) return "TECLADO";
    }
}
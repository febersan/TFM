%% DECLARAR PUERTO SERIE.
% Limpiar y borrar puertos previos en la misma direccion.
clear
clc
delete(instrfind({'Port'},{'/dev/cu.usbmodem1462401'}));
% Crear puerto serie, interrupcion y variables globales.
global rpm
rpm = 0;
global current_mA
current_mA = 0;
global avance
avance = 0;
global time_ms
time_ms = 0;
global go
go = true;
global pR
pR = true;
s = serial('/dev/cu.usbmodem1462401','BaudRate',9600,'Terminator','LF');
warning('off','MATLAB:serial:fscanf:unsuccessfulRead');
s.BytesAvailableFcn = @newdata;
% Abrir puerto serie.
fopen(s);
while pR == true
end

%% CODIGO.
while true
    if go
        prompt = ("\nESCRIBA COMANDO (END/end PARA TERMINAR): ");
        str = input(prompt,'s');
        if str == "END" || str == "end"
            fprintf("--> FINALIZANDO PROGRAMA Y CERRANDO PUERTO SERIE.\n\n");
            cerrar(s);
            break
        elseif str == "TEST" || str == "test"
            rpm = [];
            current_mA = [];
            avance = [];
            time_ms = [];
            fprintf(s,str);
            pause(0.15);
        elseif isempty(str)
            fprintf("--> COMANDO NO RECONOCIDO\n");
        else
            fprintf(s,str);
            pause(0.15);
        end
    end
end

%% FUNCION CERRAR PUERTO SERIE.
function cerrar(obj)
fclose(obj);
delete(obj);
clear obj;
end

%% FUNCION NEWDATA.
function newdata(obj,~)
global rpm
global current_mA
global avance
global time_ms
global go
global pR
data = fgetl(obj);
data = regexprep(data,"\r",""); %Borrar CR final.
data = string(data);
if isequal("NOWORK",data)
    go = true;
elseif isequal("WORK",data)
    go = false;
elseif isequal("FINTEST",data)
    str1 = input("*** INTRODUZCA NOMBRE DE ARCHIVO PARA GUARDAR LOS DATOS (N/n PARA NO GUARDAR): ",'s');
    while isempty(str1)
        str1 = input("*** NO VALIDO. INTRODUZCA NOMBRE DE ARCHIVO PARA GUARDAR LOS DATOS (N/n PARA NO GUARDAR): ",'s');
    end
    if length(rpm) >= 3
        rpm(1:2) = [];
    else
        rpm(1) = [];
    end
    if length(current_mA) >= 3
        current_mA(1:2) = [];
    else
        current_mA(1) = [];
    end
    if length(avance) >= 3
        avance(1:2) = [];
    else
        avance(1) = [];
    end
    if length(time_ms) >= 3
        time_ms(1:2) = [];
    else
        time_ms(1) = [];
    end
    rpm=medfilt1(rpm,9); %Filtro de mediana de orden 9.
    if str1 == "N" || str1 == "n"
        fprintf("--> HA ELEGIDO NO GUARDAR DATOS\n");
    else
        save(str1,'rpm','current_mA','avance','time_ms'); %Primera fila (RPM), segunda fila (CURRENT), tercera fila (AVANCE), cuarta fila (TIEMPO).
        fprintf("--> DATOS GUARDADOS\n");
    end
    fprintf("--> PINTANDO GRAFICAS\n");
    figure
    set(gcf,'position',[40,40,1600,900])
    subplot(2,2,1)
    plot(time_ms,rpm)
    axis([-inf inf -inf inf])
    title('RPM')
    ylabel('RPM')
    xlabel('Tiempo (ms)')
    subplot(2,2,2)
    plot(time_ms,current_mA)
    axis([-inf inf -inf inf])
    title('CURRENT (mA)')
    ylabel('Current (mA)')
    xlabel('Tiempo (ms)')
    subplot(2,2,3)
    plot(time_ms,avance)
    axis([-inf inf -inf inf])
    title('AVANCE LINEAL')
    ylabel('Avance Lineal')
    xlabel('Tiempo (ms)')
    subplot(2,2,4)
    plot(avance,current_mA)
    axis([-inf inf -inf inf])
    title('CURRENT (mA) VS. AVANCE LINEAL')
    ylabel('Current (mA)')
    xlabel('Avance Lineal')
    go = true;
elseif contains(data,"D1:")
    data = regexprep(data,"D1:","");
    data = str2double(data);
    rpm = [rpm data];
elseif contains(data,"D2:")
    data = regexprep(data,"D2:","");
    data = str2double(data);
    data = data*2000/1024;
    current_mA = [current_mA data];
elseif contains(data,"D3:")
    data = regexprep(data,"D3:","");
    data = str2double(data);
    avance = [avance data];
elseif contains(data,"D4:")
    data = regexprep(data,"D4:","");
    data = str2double(data);
    data = data/1000;
    time_ms = [time_ms data];
else
    if pR == false
        fprintf("--> ");
    end
    disp(data);
    if contains(data,"LISTO")
        pR = false;
    elseif ismember(data,"IMPOSIBLE")
        pR = false;
    end
end
end
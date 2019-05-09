clear all
clc
vect_var=[];
vect_voltage_a0 =[];
vect_current =[];
vect_temp_a2 =[];
vect_capacidade =[];
vect_minutos =[];
s = serial('COM3');
set(s, 'InputBufferSize', 50);
set(s, 'FlowControl', 'none');
set(s, 'BaudRate', 9600);
set(s, 'Parity', 'none');
set(s, 'DataBits', 8);
set(s, 'StopBit', 2);
set(s, 'Timeout',100);
i=1;
 
f = figure
set(f,'name','Plots','numbertitle','off')
 
fopen(s)
display(s)
 
       % while ishandle(plotGraph) %Loop when Plot is Active
while s ~= -1   
    
str_tx = fscanf(s,'s');
display(str_tx)
 
var = sscanf(str_tx, '%f V | %f mA | %f ''C | %f mAh | %d min',[1 5])
vect_var = [vect_var ; var]
 
voltage_a0 = var(1,1)
vect_voltage_a0 = [vect_voltage_a0 ; voltage_a0]
 
current = var(1,2)
vect_current = [vect_current ; current]
 
temp_a2 = var(1,3)
vect_temp_a2 = [vect_temp_a2 ; temp_a2]
 
capacidade = var(1,4)
vect_capacidade = [vect_capacidade ; capacidade]
 
minutos = var(1,5)
vect_minutos = [vect_minutos ; minutos]
 
filename = 'C:\Users\Desk\Documents\testdata.xls';
xlswrite(filename,vect_var)
 
set(f,'Units', 'Normalized', 'OuterPosition', [0 0 1 1]);
 
hold on;
graf_volt = vect_var(i,1);
graf_current = vect_var(i,2);
graf_temp = vect_var (i,3);
graf_cap = vect_var(i,4);
graf_tempo = vect_var(i,5);
 
%xlim([0 inf]);
%ylim([0 inf]);
%plot(graf_tempo,graf_cap,'-bo');
%title('Capacidade vs Tempo')
%ylabel('Capacidade (mAh)')
%xlabel('Tempo (minutos)')
%drawnow;
 
hold on;
graf1 = subplot(2,2,1)
xlim(graf1,[0 inf])
ylim(graf1,[0 inf]);
plot(graf_tempo,graf_volt,'--bo');
title('Carga vs Tempo')
ylabel('Carga (V)')
xlabel('Tempo (minutos)')
drawnow;
 
hold on;
graf2 = subplot(2,2,2)
xlim(graf2,[0 inf])
ylim(graf2,[0 inf]);
plot(graf_tempo,graf_current,'--ro');
title('Corrente de descarga vs Tempo')
ylabel('Corrente de descarga (mA)')
xlabel('Tempo (minutos)')
drawnow;
 
hold on;
graf3 = subplot(2,2,3)
xlim(graf3,[0 inf])
ylim(graf3,[0 inf]);
plot(graf_tempo,graf_cap,'--ko');
title('Capacidade vs Tempo')
ylabel('Capacidade (mAh)')
xlabel('Tempo (minutos)')
drawnow;
 
hold on;
graf4 = subplot(2,2,4)
xlim(graf4,[0 inf])
ylim(graf4,[0 inf]);
plot(graf_tempo,graf_temp,'--go');
title('Temperatura vs Tempo')
ylabel('Temperatura (ºC)')
xlabel('Tempo (minutos)')
drawnow;
 
i=i+1;
 
end

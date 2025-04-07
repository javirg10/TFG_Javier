clear; close all; clc;

%URL: https://tvgi-8700f.web.app
%Usuario: tvgi.appand@gmail.com
%Contraseña: Tvgi19&H

%Prueba de equilibrio para guardar datos -> Guardar datos y Reiniciar
%Prueba de velocidad para guardar datos -> Guardar datos y Reiniciar
%Preuba de levantadas ->Guardar datos-Guardar-no,salir

%Acceso a los datos de la prueba de Equilibrio
%clear; close all; clc;
data_raw_tBA = webread('https://tvgi-8700f.firebaseio.com/tBA.json');
total_pruebas_tBA = length(fieldnames(data_raw_tBA));
data_raw_tBA_cells = struct2cell(data_raw_tBA);
ultima_prueba_tBA = data_raw_tBA_cells(total_pruebas_tBA);
estructura_ultima_prueba_tBA = ultima_prueba_tBA{1};
dateTimeEndTest_tBA = estructura_ultima_prueba_tBA.dateTimeEndTest;
dateTimeTestDay_tBA = estructura_ultima_prueba_tBA.dateTimeTestDay;
numHistorial_tBA = estructura_ultima_prueba_tBA.numHistorial;
points_tBA = estructura_ultima_prueba_tBA.points;
pointsParalell_tBA = estructura_ultima_prueba_tBA.pointsParalell;
pointsSemitandem_tBA = estructura_ultima_prueba_tBA.pointsSemitandem;
pointsTandem_tBA = estructura_ultima_prueba_tBA.pointsTandem;
timeParalell_tBA = estructura_ultima_prueba_tBA.timeParalell;
timeSemitandem_tBA = estructura_ultima_prueba_tBA.timeSemitandem;
timeTandem_tBA = estructura_ultima_prueba_tBA.timeTandem;

fprintf('DATOS PRUEBA EQUILIBRIO\n');
fprintf('*Hora finalización test: %s.\n', dateTimeEndTest_tBA);
fprintf('*Hora inicio test: %s.\n', dateTimeTestDay_tBA);
fprintf('*Número historial: %s.\n', numHistorial_tBA);
fprintf('*Puntos totales: %d.\n', points_tBA);
fprintf('*Puntos paralelo: %d y tiempo paralelo: %d.\n', pointsParalell_tBA, timeParalell_tBA);
fprintf('*Puntos Semitandem: %d y tiempo Semitandem: %d.\n', pointsSemitandem_tBA, timeSemitandem_tBA);
fprintf('*Puntos Tandem: %d y tiempo Tandem: %d.\n', pointsTandem_tBA, timeTandem_tBA);

%Acceso a los datos de la prueba de Sentadillas
%clear; close all; clc;
data_raw_tCS = webread('https://tvgi-8700f.firebaseio.com/tCS.json');
total_pruebas_tCS = length(fieldnames(data_raw_tCS));
data_raw_tCS_cells = struct2cell(data_raw_tCS);
ultima_prueba_tCS = data_raw_tCS_cells(total_pruebas_tCS);
estructura_ultima_prueba_tCS = ultima_prueba_tCS{1};
dataChart_tCS = estructura_ultima_prueba_tCS.dataChart;
dateTimeEndTest_tCS = estructura_ultima_prueba_tCS.dateTimeEndTest;
dateTimeTestDay_tCS = estructura_ultima_prueba_tCS.dateTimeTestDay;
numHistorial_tCS = estructura_ultima_prueba_tCS.numHistorial;
numRepeatedChairStands_tCS = estructura_ultima_prueba_tCS.numRepeatedChairStands;
passed_tCS = estructura_ultima_prueba_tCS.passed;
points_tCS = estructura_ultima_prueba_tCS.points;
time_tCS = estructura_ultima_prueba_tCS.time;

fprintf('DATOS PRUEBA SENTADILLAS\n');
fprintf('*Hora finalización test: %s.\n', dateTimeEndTest_tCS);
fprintf('*Hora inicio test: %s.\n', dateTimeTestDay_tCS);
fprintf('*Número historial: %s.\n', numHistorial_tCS);
fprintf('*Puntos: %d.\n', points_tCS);
fprintf('*Tiempo: %d.\n', time_tCS);

figure
plot(dataChart_tCS);
title('Sentadillas','FontSize',12,'FontName','Cambria');
xlabel('Muestras','FontSize',12,'FontName','Cambria');
ylabel('Ángulo(º)','FontSize',12,'FontName','Cambria');



%Acceso a los datos de la prueba de Velocidad de la Marcha
%clear; close all; clc;
data_raw_tGS = webread('https://tvgi-8700f.firebaseio.com/tGS.json');
total_pruebas_tGS = length(fieldnames(data_raw_tGS));
data_raw_tGS_cells = struct2cell(data_raw_tGS);
ultima_prueba_tGS = data_raw_tGS_cells(total_pruebas_tGS);
estructura_ultima_prueba_tGS = ultima_prueba_tGS{1};
attempt_tGS = estructura_ultima_prueba_tGS.attempt;
dataChart_tGS = estructura_ultima_prueba_tGS.dataChart;
dateTimeEndTest_tGS = estructura_ultima_prueba_tGS.dateTimeEndTest;
dateTimeTestDay_tGS = estructura_ultima_prueba_tGS.dateTimeTestDay;
numHistorial_tGS = estructura_ultima_prueba_tGS.numHistorial;
passed_tGS = estructura_ultima_prueba_tGS.passed;
points_tGS = estructura_ultima_prueba_tGS.points;
presetdistance_tGS = estructura_ultima_prueba_tGS.presetdistance;
time_tGS = estructura_ultima_prueba_tGS.time;
walkdistance_tGS = estructura_ultima_prueba_tGS.walkdistance;

fprintf('DATOS PRUEBA VELOCIDAD DE LA MARCHA\n');
fprintf('*Hora finalización test: %s.\n', dateTimeEndTest_tGS);
fprintf('*Hora inicio test: %s.\n', dateTimeTestDay_tGS);
fprintf('*Número historial: %s.\n', numHistorial_tGS);
fprintf('*Puntos: %d.\n', points_tGS);
fprintf('*Tiempo: %d.\n', time_tGS);
fprintf('*Distancia recorrida: %d.\n', walkdistance_tGS);


dataChart_tGS_mat = cellfun(@str2num, dataChart_tGS);
figure
plot(dataChart_tGS_mat);
title('Distancia','FontSize',12,'FontName','Cambria');
xlabel('Muestras','FontSize',12,'FontName','Cambria');
ylabel('mm','FontSize',12,'FontName','Cambria');

num_muestras = length(dataChart_tGS);
Ts = 17; %ms
fs = 1/Ts; 

array_tiempo = 0:Ts:(num_muestras-1)*Ts;

for i=1:1:num_muestras-1
    speed(i) = (dataChart_tGS_mat(i+1)-dataChart_tGS_mat(i))/Ts;
end


figure
plot(array_tiempo(1:length(speed)), speed);
title('Velocidad','FontSize',12,'FontName','Cambria');
xlabel('ms','FontSize',12,'FontName','Cambria');
ylabel('m/s','FontSize',12,'FontName','Cambria');


% figure
% %plot(array_tiempo(1:length(speed)), speed, 'o'); xlabel('Time'); ylabel('Blood Concentration');
% f = fit(array_tiempo(1:length(speed))', speed', 'poly2');
% plot(array_tiempo(1:length(speed)), speed, '.-r')
% % plot(array_tiempo(1:length(speed)), f, '.-b')
% plot(f, array_tiempo(1:length(speed)), speed, '.-b')
% title('Velocidad','FontSize',12,'FontName','Cambria');
% xlabel('ms','FontSize',12,'FontName','Cambria');
% ylabel('m/s','FontSize',12,'FontName','Cambria');






graphics_toolkit("gnuplot");
data = dlmread('scaling_results.csv', ',', 1, 0);
cores = data(:, 1); time = data(:, 2);
speedup = time(1) ./ time;
efficiency = (speedup ./ cores) * 100;

figure('visible', 'off');
plot(cores, speedup, '-bo', 'LineWidth',2, 'MarkerFaceColor','b'); hold on;
plot(cores, cores, '--k', 'LineWidth',1);
xlabel('Processors'); ylabel('Speedup'); title('Speedup'); grid on;
print('scaling_speedup.png', '-dpng');

figure('visible', 'off');
plot(cores, efficiency, '-s', 'LineWidth',2, 'Color',[0,0.5,0], 'MarkerFaceColor','g');
xlabel('Processors'); ylabel('Efficiency (%)'); ylim([0, 110]); grid on;
print('scaling_efficiency.png', '-dpng');

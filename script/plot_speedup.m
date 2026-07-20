graphics_toolkit("gnuplot");
data = dlmread('scaling_results_tsplib_euc2d.csv', ',', 1, 0);
cores = data(:, 1);
time = data(:, 3);
speedup = time(1) ./ time;
efficiency = (speedup ./ cores) * 100;

figure('visible', 'off');
plot(cores, speedup, '-bo', 'LineWidth',2, 'MarkerFaceColor','b'); hold on;
plot(cores, cores, '--k', 'LineWidth',1);
xlabel('Processors'); ylabel('Speedup');
title('Speedup: TSPLIB EUC\_2D Benchmark'); grid on;
print('scaling_speedup_tsplib_euc2d.png', '-dpng');

figure('visible', 'off');
plot(cores, efficiency, '-s', 'LineWidth',2, 'Color',[0,0.5,0], 'MarkerFaceColor','g');
xlabel('Processors'); ylabel('Efficiency (%)'); ylim([0, 110]); grid on;
title('Parallel Efficiency: TSPLIB EUC\_2D Benchmark');
print('scaling_efficiency_tsplib_euc2d.png', '-dpng');

graphics_toolkit("gnuplot");
output_img = 'scaling_breakdown_tsplib_euc2d.png';
data = dlmread('scaling_results_tsplib_euc2d.csv', ',', 1, 0);
cores = data(:, 1);
time = data(:, 3);

ideal = time(1) ./ cores;
overhead = time - ideal;
overhead(overhead < 0) = 0;

h = figure('visible', 'off', 'Position', [0,0,900,600]);
b = bar(cores, [ideal, overhead], 0.6, 'stacked');
set(b(1), 'FaceColor', [0.27, 0.51, 0.71]);
set(b(2), 'FaceColor', [1.0, 0.49, 0.0]);

lgd = legend({'Ideal computation time', 'Measured overhead'}, 'Location', 'northeast');
set(lgd, 'FontSize', 12);

for i = 1:length(cores)
    pct = (overhead(i) / time(i)) * 100;
    if pct > 0.1
        text(cores(i), time(i), sprintf('%.1f%%', pct), ...
             'VerticalAlignment', 'bottom', 'HorizontalAlignment', 'center', ...
             'FontSize', 11, 'FontWeight', 'bold', 'Color', 'k');
    end
end

xlabel('Processors', 'FontSize', 14, 'FontWeight', 'bold');
ylabel('Time (s)', 'FontSize', 14, 'FontWeight', 'bold');
title('Runtime Decomposition: TSPLIB EUC\_2D Benchmark', 'FontSize', 16, 'FontWeight', 'bold');
ylim([0, max(time) * 1.15]);
grid on;
print(output_img, '-dpng');

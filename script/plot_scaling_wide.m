graphics_toolkit("gnuplot");
output_img = 'scaling_breakdown_fixed.png';
data = dlmread('scaling_results.csv', ',', 1, 0);
cores = data(:, 1); 
time = data(:, 2);

% Calculate Ideal vs Overhead
ideal = time(1) ./ cores;
overhead = time - ideal; 
overhead(overhead < 0) = 0; % Fix small noise

% Create Plot
h = figure('visible', 'off', 'Position', [0,0,900,600]);
b = bar(cores, [ideal, overhead], 0.6, 'stacked');

% Styling
set(b(1), 'FaceColor', [0.27, 0.51, 0.71]); % Steel Blue
set(b(2), 'FaceColor', [1.0, 0.49, 0.0]);   % Orange

% Legend (Fixed location)
lgd = legend({'Computation', 'Communication'}, 'Location', 'northeast');
set(lgd, 'FontSize', 12);

% Add Percentage Labels on Top
for i = 1:length(cores)
    % Calculate % of time spent on Overhead
    pct = (overhead(i) / time(i)) * 100;
    
    % Only show if > 0.1% to keep it clean
    if pct > 0.1
        text(cores(i), time(i), sprintf('%.1f%%', pct), ...
             'VerticalAlignment', 'bottom', 'HorizontalAlignment', 'center', ...
             'FontSize', 11, 'FontWeight', 'bold', 'Color', 'k');
    end
end

% Labels
xlabel('Processors', 'FontSize', 14, 'FontWeight', 'bold');
ylabel('Time (s)', 'FontSize', 14, 'FontWeight', 'bold');
title('Runtime Breakdown (Strong Scaling)', 'FontSize', 16, 'FontWeight', 'bold');
ylim([0, max(time) * 1.15]); % Add space on top for the text
grid on;

% Save
print(output_img, '-dpng');

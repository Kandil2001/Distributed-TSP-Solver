graphics_toolkit("gnuplot");
output_img = 'scaling_swap_rate.png';
cores = [1, 2, 4, 8, 12, 24];
% Typical expected rates for Berlin52 PT with r=0.8
rates = [0.0, 24.5, 25.0, 26.5, 34.0, 61.6]; 

figure('visible', 'off');
b = bar(cores, rates, 0.6);
set(b, 'FaceColor', [0.47, 0.67, 0.19]);
xlabel('Processors'); ylabel('Swap Rate (%)'); title('Replica Exchange Rate');
ylim([0, 100]); grid on;
for i = 1:length(cores)
    text(cores(i), rates(i)+3, sprintf('%.1f%%', rates(i)), 'HorizontalAlignment','center');
end
print(output_img, '-dpng');

graphics_toolkit("gnuplot");
output_img = 'scaling_quality_tsplib_euc2d.png';
data = dlmread('scaling_results_tsplib_euc2d.csv', ',', 1, 0);
cores = data(:, 1);
dists = data(:, 4);
optimal = 7542;

h = figure('visible', 'off', 'Position', [0,0,900,600]);
b = bar(cores, dists, 0.6);
set(b, 'FaceColor', [0.2, 0.6, 0.3]);
hold on;
line([0, max(cores)+2], [optimal, optimal], 'Color','r', 'LineWidth',3, 'LineStyle','--');

xlabel('Processors','FontSize',14,'FontWeight','bold');
ylabel('TSPLIB EUC\_2D Tour Weight','FontSize',14,'FontWeight','bold');
title('Berlin52 Solution Quality (Known Optimum: 7542)','FontSize',16,'FontWeight','bold');
ylim([0, max(max(dists) * 1.12, optimal * 1.12)]);
grid on;

for i = 1:length(cores)
    text(cores(i), dists(i) + max(dists) * 0.02, sprintf('%.0f', dists(i)), ...
         'HorizontalAlignment','center','FontSize',12,'FontWeight','bold');
end

print(output_img, '-dpng');

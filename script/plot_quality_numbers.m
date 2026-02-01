graphics_toolkit("gnuplot");
output_img = 'scaling_quality_bar.png';
data = dlmread('scaling_results.csv', ',', 1, 0);
cores = data(:, 1); dists = data(:, 3);
optimal = 7542;

h = figure('visible', 'off', 'Position', [0,0,900,600]);
b = bar(cores, dists, 0.6);
set(b, 'FaceColor', [0.2, 0.6, 0.3]);
hold on;
line([0, 30], [optimal, optimal], 'Color','r', 'LineWidth',3, 'LineStyle','--');

xlabel('Processors','FontSize',14,'FontWeight','bold');
ylabel('Best Distance','FontSize',14,'FontWeight','bold');
title('Solution Quality (Target: 7542)','FontSize',16,'FontWeight','bold');
ylim([0, 10000]); 
grid on;

for i = 1:length(cores)
    text(cores(i), dists(i)+300, sprintf('%.0f', dists(i)), 'HorizontalAlignment','center','FontSize',12,'FontWeight','bold');
end
print(output_img, '-dpng');

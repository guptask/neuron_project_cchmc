clf;
clear all;
image = '1792Ap9 DMSOa';
type = '_red_high_3layers_enhanced';
startFrame = 1;
endFrame = 12;
for i = startFrame : endFrame
    filename = [image, '\z', int2str(i), type, '.tif'];
    slice(:,:,i) = imread(filename);
end

%build the surface of constant fluro intensity
p=patch(isosurface(slice, 100));
reducepatch(p, 0.5);
set(p,'facecolor',[1,1,1],'edgecolor','none');
set(gca,'projection','perspective');
box on;
light('position',[1,1,1]);
light('position',[-1,-1,-1]);

%scale in the z direcion to compensate
%for slice thickness
zscale=0.1;
daspect([1,1,zscale]);
axis on;
set(gca,'color',[1,1,1]*0.8);
set(gca,'xlim',[1 1024], 'ylim',[1 1024]);

videofile = [image, type];
OptionZ.FrameRate=5; OptionZ.Duration=5.5; OptionZ.Periodic=true;
CaptureFigVid([0,-90;0,-45;0,0;0,45;0,90], videofile, OptionZ);

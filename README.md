# CrystalNodes
*By Skylake*

To apply the editor background material, go to the plugin content folder and find SB_GraphBackground brush, copy that brush to your editor settings. Better disable editor grids.

You can now turn off some fancy stuff at your projrct settings -> CrystalNodes.

Have fun!

## Limitations:
The material parameter of nodes now updates properly in all windows EXCEPT BP graph that's docked to the main window. 
Why? Because in this case the graph doesn't access any valid Material Parameter Collection Instance. It always use the default MPC value. We can do nothing but leave it not updated.

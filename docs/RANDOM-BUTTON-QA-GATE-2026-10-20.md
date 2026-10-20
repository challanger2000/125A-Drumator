# Random Velocity GUI validation gate

Candidate commit: 9bf72f6c3af8522248a102d6cd28315ef49bb09a.

The graphical one-shot button is not validated. Before merge, the native Windows integration test must click the button on an occupied row, verify unchanged blank steps, verify one undo, and test a legacy kit whose stored realtime random velocity is nonzero. Confirm that the legacy modulation remains visible, controllable and audible while all new kits use a one-shot control.

Do not claim Studio One acceptance from Windows simulation. Do not merge untested implementation into develop/v1.0.0.

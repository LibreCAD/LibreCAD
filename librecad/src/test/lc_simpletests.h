#ifndef LC_SIMPLETESTS_H
#define LC_SIMPLETESTS_H

class RS_EntityContainer;

struct LC_SimpleTests
{
	LC_SimpleTests() = delete;
	~LC_SimpleTests() = delete;

	/** dumps entities to file */
	static void slotTestDumpEntities(RS_EntityContainer* d=nullptr);
	/** dumps undo info to stdout */
	static void slotTestDumpUndo();
	/** updates all inserts */
	static void slotTestUpdateInserts();
	/** draws some random lines */
	static void slotTestDrawFreehand();
	/** inserts a test block */
	static void slotTestInsertBlock();
	/** inserts a test ellipse */
	static void slotTestInsertEllipse();
	/** inserts a test mtext */
	static void slotTestInsertMText();
	/** inserts a test text */
	static void slotTestInsertText();
	/** inserts a test image */
	static void slotTestInsertImage();
	/** unicode table */
	static void slotTestUnicode();
	/** math experimental */
	static void slotTestMath01();
	/** resizes window to 640x480 for screen shots */
	static void slotTestResize640();
	/** resizes window to 640x480 for screen shots */
	static void slotTestResize800();
	/** resizes window to 640x480 for screen shots */
	static void slotTestResize1024();
};

#endif // LC_SIMPLETESTS_H

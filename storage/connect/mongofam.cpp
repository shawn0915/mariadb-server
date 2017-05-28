/************ MONGO FAM C++ Program Source Code File (.CPP) ************/
/* PROGRAM NAME: mongofam.cpp                                          */
/* -------------                                                       */
/*  Version 1.1                                                        */
/*                                                                     */
/* COPYRIGHT:                                                          */
/* ----------                                                          */
/*  (C) Copyright to the author Olivier BERTRAND          20017        */
/*                                                                     */
/* WHAT THIS PROGRAM DOES:                                             */
/* -----------------------                                             */
/*  This program are the MongoDB access method classes.                */
/*                                                                     */
/***********************************************************************/

/***********************************************************************/
/*  Include relevant sections of the System header files.              */
/***********************************************************************/
#include "my_global.h"
#if defined(__WIN__)
//#include <io.h>
//#include <fcntl.h>
//#include <errno.h>
#if defined(__BORLANDC__)
#define __MFC_COMPAT__                   // To define min/max as macro
#endif   // __BORLANDC__
//#include <windows.h>
#else   // !__WIN__
#if defined(UNIX) || defined(UNIV_LINUX)
//#include <errno.h>
#include <unistd.h>
//#if !defined(sun)                      // Sun has the ftruncate fnc.
//#define USETEMP                        // Force copy mode for DELETE
//#endif   // !sun
#else   // !UNIX
//#include <io.h>
#endif  // !UNIX
//#include <fcntl.h>
#endif  // !__WIN__

/***********************************************************************/
/*  Include application header files:                                  */
/*  global.h    is header containing all global declarations.          */
/*  plgdbsem.h  is header containing the DB application declarations.  */
/*  filamtxt.h  is header containing the file AM classes declarations. */
/***********************************************************************/
#include "global.h"
#include "plgdbsem.h"
#include "reldef.h"
#include "filamtxt.h"
#include "tabdos.h"
#include "tabjson.h"
#include "mongofam.h"

#if defined(UNIX) || defined(UNIV_LINUX)
#include "osutil.h"
//#define _fileno fileno
//#define _O_RDONLY O_RDONLY
#endif

// Required to initialize libmongoc's internals
void mongo_init(bool init)
{
	if (init)
		mongoc_init();
	else
		mongoc_cleanup();

}	// end of mongo_init

/* --------------------------- Class MGOFAM -------------------------- */

/***********************************************************************/
/*  Constructors.                                                      */
/***********************************************************************/
MGOFAM::MGOFAM(PJDEF tdp) : DOSFAM((PDOSDEF)NULL)
{
	Client = NULL;
	Database = NULL;
	Collection = NULL;
	Cursor = NULL;
	Query = NULL;
	Opts = NULL;
	To_Fbt = NULL;
	Mode = MODE_ANY;
	Uristr = tdp->Uri;
	Db_name = tdp->Schema;
	Coll_name = tdp->Collname;
	Options = tdp->Options;
	Filter = NULL;
	Done = false;
	Pipe = tdp->Pipe;
	Lrecl = tdp->Lrecl + tdp->Ending;
} // end of MGOFAM standard constructor
 
 MGOFAM::MGOFAM(PMGOFAM tdfp) : DOSFAM(tdfp)
{
	Client = tdfp->Client;
	Database = NULL;
	Collection = tdfp->Collection;
	Cursor = tdfp->Cursor;
	Query = tdfp->Query;
	Opts = tdfp->Opts;
	To_Fbt = tdfp->To_Fbt;
	Mode = tdfp->Mode;
	Uristr = tdfp->Uristr;
	Db_name = tdfp->Db_name;
	Coll_name = tdfp->Coll_name;
	Options = tdfp->Options;
	Filter = NULL;
	Done = tdfp->Done;
	Pipe = tdfp->Pipe;
 } // end of MGOFAM copy constructor

#if 0
void *MGOFAM::mgo_alloc(size_t n)
{
  char *mst = (char*)PlgDBSubAlloc(G, NULL, n + sizeof(size_t));

	if (mst) {
	  *(size_t*)mst = n;
	  return mst + sizeof(size_t);
	} // endif mst

	return NULL;
} // end of mgo_alloc

void *MGOFAM::mgo_calloc(size_t n, size_t sz)
{
  void *m = mgo_alloc(n * sz);

  if (m)
	  memset(m, 0, n * sz);

  return m;
} // end of mgo_calloc

void *MGOFAM::mgo_realloc(void *m, size_t n)
{
	if (!m)
		return n ? mgo_alloc(n) : NULL;

	size_t *osz = (size_t*)((char*)m - sizeof(size_t));

	if (n > *osz) {
		void *nwm = mgo_alloc(n);

		if (nwm)
			memcpy(nwm, m, *osz);

		return nwm;
	} else {
		*osz = n;
		return m;
	}	// endif n

} // end of mgo_realloc
#endif // 0

/***********************************************************************/
/*  Reset: reset position values at the beginning of file.             */
/***********************************************************************/
void MGOFAM::Reset(void)
{
	TXTFAM::Reset();
	Fpos = Tpos = Spos = 0;
} // end of Reset

/***********************************************************************/
/*  MGO GetFileLength: returns file size in number of bytes.           */
/***********************************************************************/
int MGOFAM::GetFileLength(PGLOBAL g)
{
	return 0;
} // end of GetFileLength

/***********************************************************************/
/*  Cardinality: returns table cardinality in number of rows.          */
/*  This function can be called with a null argument to test the       */
/*  availability of Cardinality implementation (1 yes, 0 no).          */
/***********************************************************************/
int MGOFAM::Cardinality(PGLOBAL g)
{
	if (g) {
		if (!Init(g)) {
			bson_t     *query;
			const char *jf = NULL;

			if (Pipe)
				return 10;
			else if (Filter)
				jf = Filter;

			if (jf) {
				query = bson_new_from_json((const uint8_t *)jf, -1, &Error);

				if (!query) {
					htrc("Wrong filter: %s", Error.message);
					return 10;
				}	// endif Query

			} else
				query = bson_new();

			int64_t card = (int)mongoc_collection_count(Collection,
				MONGOC_QUERY_NONE, query, 0, 0, NULL, &Error);

			if (card < 0)
				sprintf(g->Message, "Collection count: %s", Error.message);

			bson_destroy(query);
			return card;
		}	else
			return -1;

	} else
		return 1;

} // end of Cardinality

/***********************************************************************/
/*  Note: This function is not really implemented yet.                 */
/***********************************************************************/
int MGOFAM::MaxBlkSize(PGLOBAL, int s)
{
	return s;
} // end of MaxBlkSize

/***********************************************************************/
/*  Init: initialize MongoDB processing.                               */
/***********************************************************************/
bool MGOFAM::Init(PGLOBAL g)
{
	if (Done)
		return false;

	if (Options && !Pipe) {
		char *p = (char*)strchr(Options, ';');

		if (p) {
			*p++ = 0;

			if (p)
				Filter = p;

		} // endif p

	} // endif Options

	Uri = mongoc_uri_new(Uristr);

	if (!Uri) {
		sprintf(g->Message, "Failed to parse URI: \"%s\"", Uristr);
		return true;
	}	// endif Uri

		// Create a new client pool instance
	Pool = mongoc_client_pool_new(Uri);
	mongoc_client_pool_set_error_api(Pool, 2);

	// Register the application name so we can track it in the profile logs
	// on the server. This can also be done from the URI.
	mongoc_client_pool_set_appname(Pool, "Connect");

	// Create a new client instance
	Client = mongoc_client_pool_pop(Pool);
	//Client = mongoc_client_new(uristr);

	if (!Client) {
		sprintf(g->Message, "Failed to get Client");
		return true;
	}	// endif Client

		//mongoc_client_set_error_api(Client, 2);

		// Register the application name so we can track it in the profile logs
		// on the server. This can also be done from the URI.
		//mongoc_client_set_appname(Client, "Connect");

		// Get a handle on the database Db_name and collection Coll_name
		//	Database = mongoc_client_get_database(Client, Db_name);
		//	Collection = mongoc_database_get_collection(Database, Coll_name);
	Collection = mongoc_client_get_collection(Client, Db_name, Coll_name);

	if (!Collection) {
		sprintf(g->Message, "Failed to get Collection %s.%s", Db_name, Coll_name);
		return true;
	}	// endif Collection

	Done = true;
	return false;
} // end of Init

/***********************************************************************/
/*  OpenDB: Data Base open routine for MONGO access method.            */
/***********************************************************************/
bool MGOFAM::MakeCursor(PGLOBAL g)
{
	const char *p;
	bool  b = false, id = (Mode != MODE_READ), all = false;
	uint  len;
	PSZ   jp;
	PCOL  cp;
	PSTRG s = NULL;

	if (Options && !stricmp(Options, "all")) {
		Options = NULL;
		all = true;
	} // endif Options

	for (cp = Tdbp->GetColumns(); cp; cp = cp->GetNext())
		if (!strcmp(cp->GetName(), "_id"))
			id = true;
		else if (cp->GetFmt() && !strcmp(cp->GetFmt(), "*"))
			all = true;

	if (Pipe) {
		if (trace)
			htrc("Pipeline: %s\n", Options);

		p = strrchr(Options, ']');

		if (!p) {
			strcpy(g->Message, "Missing ] in pipeline");
			return true;
		} else
			*(char*)p = 0;

		s = new(g) STRING(g, 1023, (PSZ)Options);
		len = s->GetLength();

		if (Tdbp->GetFilter()) {
			s->Append(",{\"$match\":");

			if (!Tdbp->GetFilter()->MakeSelector(g, s, false)) {
				s->Append('}');
				Tdbp->SetFilter(NULL);     // Not needed anymore
			} else {
				if (((TDBJSN*)Tdbp)->Xcol)
					Tdbp->SetFilter(NULL);   // Incompatible

				htrc("Failed making selector\n");
				s->Truncate(len);
			} // endif Selector

		} // endif To_Filter

		if (!all) {
			// Project list
			len = s->GetLength();

			if (Tdbp->GetColumns()) {
				s->Append(",{\"$project\":{\"");

				if (!id)
					s->Append("_id\":0,\"");

				for (PCOL cp = Tdbp->GetColumns(); cp; cp = cp->GetNext()) {
					if (b)
						s->Append(",\"");
					else
						b = true;

					if ((jp = ((PJCOL)cp)->GetJpath(g, true)))
						s->Append(jp);
					else {
						s->Truncate(len);
						goto nop;
					}	// endif Jpath

					s->Append("\":1");
				} // endfor cp

			} else
				s->Append(",{\"$project\":{\"_id\":1}}");

			s->Append("}}");
		} // endif all

	 nop:
		s->Append("]}");
		s->Resize(s->GetLength() + 1);
		p = s->GetStr();

		if (trace)
			htrc("New Pipeline: %s\n", p);

		Query = bson_new_from_json((const uint8_t *)p, -1, &Error);

		if (!Query) {
			sprintf(g->Message, "Wrong pipeline: %s", Error.message);
			return true;
		}	// endif Query

		Cursor = mongoc_collection_aggregate(Collection, MONGOC_QUERY_NONE,
			Query, NULL, NULL);

		if (mongoc_cursor_error(Cursor, &Error)) {
			sprintf(g->Message, "Mongo aggregate Failure: %s", Error.message);
			return true;
		} // endif cursor

	} else {
		if (Filter || Tdbp->GetFilter()) {
			if (trace) {
				if (Filter)
					htrc("Filter: %s\n", Filter);

				if (Tdbp->GetFilter()) {
					char buf[512];

					Tdbp->GetFilter()->Prints(g, buf, 511);
					htrc("To_Filter: %s\n", buf);
				} // endif To_Filter

			}	// endif trace

			s = new(g) STRING(g, 1023, (PSZ)Filter);
			len = s->GetLength();

			if (Tdbp->GetFilter()) {
				if (Filter)
					s->Append(',');

				if (Tdbp->GetFilter()->MakeSelector(g, s, false)) {
					if (((TDBJSN*)Tdbp)->Xcol)
						Tdbp->SetFilter(NULL);   // Incompatible

					htrc("Cannot make selector\n");
					s->Truncate(len);
				}	else
					Tdbp->SetFilter(NULL);     // Not needed anymore

			} // endif To_Filter

			if (trace)
				htrc("selector: %s\n", s->GetStr());

			s->Resize(s->GetLength() + 1);

			if (s->GetLength()) {
				Query = bson_new_from_json((const uint8_t *)s->GetStr(), -1, &Error);

				if (!Query) {
					sprintf(g->Message, "Wrong filter: %s", Error.message);
					return true;
				}	// endif Query

			}	else
				Query = bson_new();

		} else
			Query = bson_new();

		if (!all) {
			if (Options && *Options) {
				if (trace)
					htrc("options=%s\n", Options);

				p = Options;
			} else if (Tdbp->GetColumns()) {
				// Projection list
				if (s)
					s->Set("{\"projection\":{\"");
				else
					s = new(g) STRING(g, 511, "{\"projection\":{\"");

				if (!id)
					s->Append("_id\":0,\"");

				for (PCOL cp = Tdbp->GetColumns(); cp; cp = cp->GetNext()) {
					if (b)
						s->Append(",\"");
					else
						b = true;

					if ((jp = ((PJCOL)cp)->GetJpath(g, true)))
						s->Append(jp);
					else {
						s->Reset();
						s->Resize(0);
						goto nope;
					}	// endif Jpath

					s->Append("\":1");
				} // endfor cp

				s->Append("}}");
				s->Resize(s->GetLength() + 1);
				p = s->GetStr();
			} else {
				// count(*)	?
				p = "{\"projection\":{\"_id\":1}}";
			} // endif Options

			Opts = bson_new_from_json((const uint8_t *)p, -1, &Error);

			if (!Opts) {
				sprintf(g->Message, "Wrong options: %s", Error.message);
				return true;
			} // endif Opts

		} // endif all

	nope:
		Cursor = mongoc_collection_find_with_opts(Collection, Query, Opts, NULL);
	} // endif Pipe

	return false;
} // end of MakeCursor

/***********************************************************************/
/*  OpenTableFile: Open a MongoDB table.                               */
/***********************************************************************/
bool MGOFAM::OpenTableFile(PGLOBAL g)
{
	Mode = Tdbp->GetMode();

	if (Pipe && Mode != MODE_READ) {
		strcpy(g->Message, "Pipeline tables are read only");
		return true;
	}	// endif Pipe

	if (Init(g))
		return true;

  if (Mode == MODE_DELETE && !Tdbp->GetNext()) {
		// Store the number of deleted lines
		DelRows = Cardinality(g);

		// Delete all documents
		if (!mongoc_collection_remove(Collection, MONGOC_REMOVE_NONE,
			                            Query, NULL, &Error)) {
			sprintf(g->Message, "Remove all: %s", Error.message);
			return true;
		}	// endif remove

	} else if (Mode != MODE_INSERT)
		if (MakeCursor(g))
			return true;

	return false;
} // end of OpenTableFile

/***********************************************************************/
/*  GetRowID: return the RowID of last read record.                    */
/***********************************************************************/
int MGOFAM::GetRowID(void)
{
	return Rows;
} // end of GetRowID

/***********************************************************************/
/*  GetPos: return the position of last read record.                   */
/***********************************************************************/
int MGOFAM::GetPos(void)
{
	return Fpos;
} // end of GetPos

/***********************************************************************/
/*  GetNextPos: return the position of next record.                    */
/***********************************************************************/
int MGOFAM::GetNextPos(void)
{
	return Fpos;						// TODO
} // end of GetNextPos

/***********************************************************************/
/*  SetPos: Replace the table at the specified position.               */
/***********************************************************************/
bool MGOFAM::SetPos(PGLOBAL g, int pos)
{
	Fpos = pos;
	Placed = true;
	return false;
} // end of SetPos

/***********************************************************************/
/*  Record file position in case of UPDATE or DELETE.                  */
/***********************************************************************/
bool MGOFAM::RecordPos(PGLOBAL g)
{
	strcpy(g->Message, "MGOFAM::RecordPos NIY");
	return true;
} // end of RecordPos

/***********************************************************************/
/*  Initialize Fpos and the current position for indexed DELETE.       */
/***********************************************************************/
int MGOFAM::InitDelete(PGLOBAL g, int fpos, int spos)
{
	strcpy(g->Message, "MGOFAM::InitDelete NIY");
	return RC_FX;
} // end of InitDelete

/***********************************************************************/
/*  Skip one record in file.                                           */
/***********************************************************************/
int MGOFAM::SkipRecord(PGLOBAL g, bool header)
{
	return RC_OK;                  // Dummy
} // end of SkipRecord

/***********************************************************************/
/*  Use to trace restaurants document contains.                        */
/***********************************************************************/
void MGOFAM::ShowDocument(bson_iter_t *iter, const bson_t *doc, const char *k)
{

	if (!doc || bson_iter_init(iter, doc)) {
		const char *key;

		while (bson_iter_next(iter)) {
			key = bson_iter_key(iter);
			htrc("Found element key: \"%s\"\n", key);

			if (BSON_ITER_HOLDS_UTF8(iter))
				htrc("%s.%s=\"%s\"\n", k, key, bson_iter_utf8(iter, NULL));
			else if (BSON_ITER_HOLDS_INT32(iter))
				htrc("%s.%s=%d\n", k, key, bson_iter_int32(iter));
			else if (BSON_ITER_HOLDS_INT64(iter))
				htrc("%s.%s=%lld\n", k, key, bson_iter_int64(iter));
			else if (BSON_ITER_HOLDS_DOUBLE(iter))
				htrc("%s.%s=%g\n", k, key, bson_iter_double(iter));
			else if (BSON_ITER_HOLDS_DATE_TIME(iter))
				htrc("%s.%s=date(%lld)\n", k, key, bson_iter_date_time(iter));
			else if (BSON_ITER_HOLDS_OID(iter)) {
				char str[25];

				bson_oid_to_string(bson_iter_oid(iter), str);
				htrc("%s.%s=%s\n", k, key, str);
			} else if (BSON_ITER_HOLDS_DECIMAL128(iter)) {
				char             *str = NULL;
				bson_decimal128_t dec;

				bson_iter_decimal128(iter, &dec);
				bson_decimal128_to_string(&dec, str);
				htrc("%s.%s=%s\n", k, key, str);
			} else if (BSON_ITER_HOLDS_DOCUMENT(iter)) {
				bson_iter_t child;

				if (bson_iter_recurse(iter, &child))
					ShowDocument(&child, NULL, key);

			} else if (BSON_ITER_HOLDS_ARRAY(iter)) {
				bson_t				*arr;
				bson_iter_t    itar;
				const uint8_t *data = NULL;
				uint32_t       len = 0;

				bson_iter_array(iter, &len, &data);
				arr = bson_new_from_data(data, len);
				ShowDocument(&itar, arr, key);
			}	// endif's

		}	// endwhile bson_iter_next

	} // endif bson_iter_init

} // end of ShowDocument

/***********************************************************************/
/*  ReadBuffer: Get next document from a collection.                   */
/***********************************************************************/
int MGOFAM::ReadBuffer(PGLOBAL g)
{
	int rc = RC_OK;

	if (mongoc_cursor_next(Cursor, &Document)) {
		char *str = bson_as_json(Document, NULL);

		if (trace > 1) {
			bson_iter_t iter;
			ShowDocument(&iter, Document, "");
		} else if (trace == 1)
			htrc("%s\n", str);

		strncpy(Tdbp->GetLine(), str, Lrecl);
		bson_free(str);
	} else if (mongoc_cursor_error(Cursor, &Error)) {
		sprintf(g->Message, "Mongo Cursor Failure: %s", Error.message);
		rc = RC_FX;
	} else {
		//mongoc_cursor_destroy(Cursor);
		rc = RC_EF;
	}	// endif's Cursor

	return rc;
} // end of ReadBuffer

/***********************************************************************/
/*  WriteBuffer: File write routine for MGO access method.             */
/***********************************************************************/
int MGOFAM::WriteBuffer(PGLOBAL g)
{
	int     rc = RC_OK;
	bson_t *doc;

	//if (Mode == MODE_INSERT && !Collection) {
	//	if ((Database = mongoc_client_get_database(Client, db_name)))
	//	  Collection = mongoc_database_create_collection(Database, coll_name,
	//		                                               NULL, &Error);

	//	if (!Collection)
	//		if (Database) {
	//			sprintf(g->Message, "Create collection: %s", Error.message);
	//			return RC_FX;
	//		} else {
	//			sprintf(g->Message, "Fail to get database %s", db_name);
	//			return RC_FX;
	//		}	// endif Database

	//}	// endif Collection

	if (!(doc = bson_new_from_json((const uint8_t *)Tdbp->GetLine(),
		-1, &Error))) {
		sprintf(g->Message, "Wrong document: %s", Error.message);
		return RC_FX;
	} // endif doc

	if (Mode != MODE_INSERT) {
		bool b = false;
		bson_iter_t iter;
		bson_t     *selector = bson_new();

		bson_iter_init(&iter, Document);

		if (bson_iter_find(&iter, "_id")) {
			if (BSON_ITER_HOLDS_OID(&iter))
				b = BSON_APPEND_OID(selector, "_id", bson_iter_oid(&iter));
			else if (BSON_ITER_HOLDS_INT32(&iter))
				b = BSON_APPEND_INT32(selector, "_id", bson_iter_int32(&iter));
			else if (BSON_ITER_HOLDS_INT64(&iter))
				b = BSON_APPEND_INT64(selector, "_id", bson_iter_int64(&iter));
			else if (BSON_ITER_HOLDS_DOUBLE(&iter))
				b = BSON_APPEND_DOUBLE(selector, "_id", bson_iter_double(&iter));
			else if (BSON_ITER_HOLDS_UTF8(&iter))
				b = BSON_APPEND_UTF8(selector, "_id", bson_iter_utf8(&iter, NULL));

		} // endif iter

		if (!b) {
			strcpy(g->Message, "Cannot find _id");
			return RC_FX;
		}	// endif oid

		if (Mode == MODE_DELETE) {
			if (!mongoc_collection_remove(Collection, MONGOC_REMOVE_NONE,
				selector, NULL, &Error)) {
				sprintf(g->Message, "Remove: %s", Error.message);
				bson_destroy(selector);
				return RC_FX;
			}	// endif remove

		} else {
			if (!mongoc_collection_update(Collection, MONGOC_UPDATE_NONE,
				selector, doc, NULL, &Error)) {
				sprintf(g->Message, "Update: %s", Error.message);
				bson_destroy(selector);
				return RC_FX;
			}	// endif remove

		}	// endif Mode

		bson_destroy(selector);
	}	else if (!mongoc_collection_insert(Collection, MONGOC_INSERT_NONE,
		                                   doc, NULL, &Error)) {
		sprintf(g->Message, "Inserting: %s", Error.message);
		rc = RC_FX;
	}	// endif insert

	bson_destroy(doc);
	return rc;
} // end of WriteBuffer

/***********************************************************************/
/*  Data Base delete line routine for MGO and BLK access methods.      */
/***********************************************************************/
int MGOFAM::DeleteRecords(PGLOBAL g, int irc)
{
	return (irc == RC_OK) ? WriteBuffer(g) : RC_OK;
} // end of DeleteRecords

/***********************************************************************/
/*  Table file close routine for MGO access method.                    */
/***********************************************************************/
void MGOFAM::CloseTableFile(PGLOBAL g, bool)
{
	if (Query) bson_destroy(Query);
	if (Opts) bson_destroy(Opts);
	if (Cursor)	mongoc_cursor_destroy(Cursor);
	if (Collection) mongoc_collection_destroy(Collection);
	//	mongoc_database_destroy(Database);
	//	mongoc_client_destroy(Client);
	if (Client) mongoc_client_pool_push(Pool, Client);
	if (Pool) mongoc_client_pool_destroy(Pool);
	if (Uri) mongoc_uri_destroy(Uri);
//bson_mem_restore_vtable();
//mongoc_cleanup();
//G = NULL;
	Done = false;
} // end of CloseTableFile

/***********************************************************************/
/*  Rewind routine for MGO access method.                              */
/***********************************************************************/
void MGOFAM::Rewind(void)
{
	mongoc_cursor_t *cursor = mongoc_cursor_clone(Cursor);

	mongoc_cursor_destroy(Cursor);
	Cursor = cursor;
} // end of Rewind

